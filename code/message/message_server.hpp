#pragma once
#include"log.hpp"
#include"etcd.hpp"
#include"message.pb.h"
#include<brpc/server.h>
#include"channel.hpp"
#include"mysql_message.hpp"
#include"es_message.hpp"
#include"rabbitmq.hpp"
#include"user_service_client.hpp"
#include"file_service_client.hpp"
namespace MindbniM
{
    class MessageServiceImpl : public MsgStorageService
    {
    public:
        MessageServiceImpl(std::shared_ptr<odb::mysql::database> mysql,std::shared_ptr<elasticlient::Client> es,MQClient::ptr mq, ServiceManager::ptr sm,std::string& exchange_name, const std::string &routing_key)
        {
            _es=std::make_shared<ESMessage>(es);
            _mysql=std::make_shared<MessageTable>(mysql);
            _service_manager=sm;
            _mq=mq;
            _exchange_name=exchange_name;
            _routing_key=routing_key;
        }
        void GetHistoryMsg(google::protobuf::RpcController* controller,const GetHistoryMsgReq* request,GetHistoryMsgRsp* response,google::protobuf::Closure* done)
        {
            brpc::ClosureGuard guard(done);
            auto errctl=[request,response](const std::string& err)
            {
                response->set_request_id(request->request_id());
                response->set_errmsg(err);
            };
            //1. 提取请求中的关键要素：请求ID，会话ID, 时间范围
            std::string ssid=request->chat_session_id();
            boost::posix_time::ptime stime=boost::posix_time::from_time_t(request->start_time());
            boost::posix_time::ptime etime=boost::posix_time::from_time_t(request->over_time());
            //2. 从数据库，获取最近的消息元信息
            std::vector<Message> msg_lists=_mysql->get_message(ssid,stime,etime);
            if(msg_lists.empty())
            {
                response->set_request_id(request->request_id());
                response->set_success(true);
                return ;
            }
            //3. 统计所有消息中文件类型消息的文件ID列表和用户ID列表
            std::unordered_set<std::string> file_id_lists;
            std::unordered_set<std::string> user_id_lists; 
            for (const auto &msg : msg_lists) 
            {
                if (msg.file_id().empty()) continue;
                LOG_ROOT_DEBUG<<"需要下载的文件ID: "<<msg.file_id();
                file_id_lists.insert(msg.file_id());
                user_id_lists.insert(msg.user_id());
            }
            //4. 从文件存储子服务中批量下载文件
            std::vector<std::string> file_ids(file_id_lists.begin(), file_id_lists.end());
            std::shared_ptr<brpc::Channel> channel = _service_manager->choose(_file_service_name);
            if(channel==nullptr)
            {
                LOG_ROOT_ERROR<<"文件存储子服务未找到";
                errctl("文件存储子服务未找到");
                return ;
            }
            FileServiceClient file_client(channel);
            std::vector<std::string> file_contents;
            bool ret=file_client.GetMultiFile(file_ids,file_contents);
            if(!ret)
            {
                LOG_ROOT_ERROR<<"文件下载失败";
                errctl("文件下载失败");
                return ;
            }
            std::unordered_map<std::string, std::string> file_data_lists;
            for(int i=0;i<file_ids.size();i++)
            {
                file_data_lists[file_ids[i]]=file_contents[i];
            }
            //5. 从用户子服务进行批量用户信息获取
            std::vector<std::string> user_ids(user_id_lists.begin(), user_id_lists.end());
            channel = _service_manager->choose(_user_service_name);
            if(channel==nullptr)
            {
                LOG_ROOT_ERROR<<"用户子服务未找到";
                return errctl("用户子服务未找到");
            }
            UserServiceClient user_client(channel);
            std::vector<UserInfo> user_infos;
            ret=user_client.GetMultiUserInfo(user_ids,user_infos);
            if(!ret)
            {
                LOG_ROOT_ERROR<<"用户信息获取失败";
                return errctl("用户信息获取失败");
            }
            std::unordered_map<std::string, UserInfo> user_lists;
            for(int i=0;i<user_ids.size();i++)
            {
                user_lists[user_ids[i]]=user_infos[i];
            }

            //6. 组织响应
            response->set_request_id(request->request_id());
            response->set_success(true);
            for (const auto &msg : msg_lists) 
            {
                auto message_info = response->add_msg_list();
                message_info->set_message_id(msg.message_id());
                message_info->set_chat_session_id(msg.session_id());
                message_info->set_timestamp(boost::posix_time::to_time_t(msg.create_time()));
                message_info->mutable_sender()->CopyFrom(user_lists[msg.user_id()]);
                switch(msg.message_type()) 
                {
                    case MessageType::STRING:
                    {
                        message_info->mutable_message()->set_message_type(MessageType::STRING);
                        message_info->mutable_message()->mutable_string_message()->set_content(msg.content());
                        break;
                    }
                    case MessageType::IMAGE:
                    {
                        message_info->mutable_message()->set_message_type(MessageType::IMAGE);
                        message_info->mutable_message()->mutable_image_message()->set_file_id(msg.file_id());
                        message_info->mutable_message()->mutable_image_message()->set_image_content(file_data_lists[msg.file_id()]);
                        break;
                    }
                    case MessageType::FILE:
                    {
                        message_info->mutable_message()->set_message_type(MessageType::FILE);
                        message_info->mutable_message()->mutable_file_message()->set_file_id(msg.file_id());
                        message_info->mutable_message()->mutable_file_message()->set_file_size(msg.file_size());
                        message_info->mutable_message()->mutable_file_message()->set_file_name(msg.file_name());
                        message_info->mutable_message()->mutable_file_message()->set_file_contents(file_data_lists[msg.file_id()]);
                        break;
                    }
                    case MessageType::SPEECH:
                    {
                        message_info->mutable_message()->set_message_type(MessageType::SPEECH);
                        message_info->mutable_message()->mutable_speech_message()->set_file_id(msg.file_id());
                        message_info->mutable_message()->mutable_speech_message()->set_file_contents(file_data_lists[msg.file_id()]);
                        break;
                    }
                    default:
                    {
                        LOG_ERROR("消息类型错误！！");
                        return;
                    }
                }
            }
        }
        void GetRecentMsg(google::protobuf::RpcController* controller,const GetRecentMsgReq* request,GetRecentMsgRsp* response,google::protobuf::Closure* done)
        {
            brpc::ClosureGuard guard(done);
            auto errctl=[request,response](const std::string& err)
            {
                response->set_request_id(request->request_id());
                response->set_errmsg(err);
            };
            //1. 提取请求中的关键要素：请求ID，会话ID, 消息数量
            std::string ssid=request->chat_session_id();
            int count=request->msg_count();
            //2. 从数据库，获取最近的消息元信息
            std::vector<Message> msg_lists=_mysql->get_message(ssid,count);
            if(msg_lists.empty())
            {
                response->set_request_id(request->request_id());
                response->set_success(true);
                return ;
            }
            //3. 统计所有消息中文件类型消息的文件ID列表和用户ID列表
            std::unordered_set<std::string> file_id_lists;
            std::unordered_set<std::string> user_id_lists; 
            for (const auto &msg : msg_lists) 
            {
                if (msg.file_id().empty()) continue;
                LOG_ROOT_DEBUG<<"需要下载的文件ID: "<<msg.file_id();
                file_id_lists.insert(msg.file_id());
                user_id_lists.insert(msg.user_id());
            }
            //4. 从文件存储子服务中批量下载文件
            std::vector<std::string> file_ids(file_id_lists.begin(), file_id_lists.end());
            std::shared_ptr<brpc::Channel> channel = _service_manager->choose(_file_service_name);
            if(channel==nullptr)
            {
                LOG_ROOT_ERROR<<"文件存储子服务未找到";
                errctl("文件存储子服务未找到");
                return ;
            }
            FileServiceClient file_client(channel);
            std::vector<std::string> file_contents;
            bool ret=file_client.GetMultiFile(file_ids,file_contents);
            if(!ret)
            {
                LOG_ROOT_ERROR<<"文件下载失败";
                errctl("文件下载失败");
                return ;
            }
            std::unordered_map<std::string, std::string> file_data_lists;
            for(int i=0;i<file_ids.size();i++)
            {
                file_data_lists[file_ids[i]]=file_contents[i];
            }
            //5. 从用户子服务进行批量用户信息获取
            std::vector<std::string> user_ids(user_id_lists.begin(), user_id_lists.end());
            channel = _service_manager->choose(_user_service_name);
            if(channel==nullptr)
            {
                LOG_ROOT_ERROR<<"用户子服务未找到";
                return errctl("用户子服务未找到");
            }
            UserServiceClient user_client(channel);
            std::vector<UserInfo> user_infos;
            ret=user_client.GetMultiUserInfo(user_ids,user_infos);
            if(!ret)
            {
                LOG_ROOT_ERROR<<"用户信息获取失败";
                return errctl("用户信息获取失败");
            }
            std::unordered_map<std::string, UserInfo> user_lists;
            for(int i=0;i<user_ids.size();i++)
            {
                user_lists[user_ids[i]]=user_infos[i];
            }

            //6. 组织响应
            response->set_request_id(request->request_id());
            response->set_success(true);
            for (const auto &msg : msg_lists) 
            {
                auto message_info = response->add_msg_list();
                message_info->set_message_id(msg.message_id());
                message_info->set_chat_session_id(msg.session_id());
                message_info->set_timestamp(boost::posix_time::to_time_t(msg.create_time()));
                message_info->mutable_sender()->CopyFrom(user_lists[msg.user_id()]);
                switch(msg.message_type()) 
                {
                    case MessageType::STRING:
                    {
                        message_info->mutable_message()->set_message_type(MessageType::STRING);
                        message_info->mutable_message()->mutable_string_message()->set_content(msg.content());
                        break;
                    }
                    case MessageType::IMAGE:
                    {
                        message_info->mutable_message()->set_message_type(MessageType::IMAGE);
                        message_info->mutable_message()->mutable_image_message()->set_file_id(msg.file_id());
                        message_info->mutable_message()->mutable_image_message()->set_image_content(file_data_lists[msg.file_id()]);
                        break;
                    }
                    case MessageType::FILE:
                    {
                        message_info->mutable_message()->set_message_type(MessageType::FILE);
                        message_info->mutable_message()->mutable_file_message()->set_file_id(msg.file_id());
                        message_info->mutable_message()->mutable_file_message()->set_file_size(msg.file_size());
                        message_info->mutable_message()->mutable_file_message()->set_file_name(msg.file_name());
                        message_info->mutable_message()->mutable_file_message()->set_file_contents(file_data_lists[msg.file_id()]);
                        break;
                    }
                    case MessageType::SPEECH:
                    {
                        message_info->mutable_message()->set_message_type(MessageType::SPEECH);
                        message_info->mutable_message()->mutable_speech_message()->set_file_id(msg.file_id());
                        message_info->mutable_message()->mutable_speech_message()->set_file_contents(file_data_lists[msg.file_id()]);
                        break;
                    }
                    default:
                    {
                        LOG_ERROR("消息类型错误！！");
                        return;
                    }
                }
            }
        }
        void MsgSearch(google::protobuf::RpcController* controller,const MsgSearchReq* request,MsgSearchRsp* response,google::protobuf::Closure* done)
        {
            brpc::ClosureGuard guard(done);
            auto errctl=[request,response](const std::string& err)
            {
                response->set_request_id(request->request_id());
                response->set_errmsg(err);
            };
            //1. 提取请求中的关键要素：请求ID，会话ID, 搜索关键字
            std::string ssid=request->chat_session_id();
            std::string key=request->search_key();
            //2. 从es获取最近的消息元信息
            std::vector<Message> msg_lists=_es->Search(key,ssid);
            if(msg_lists.empty())
            {
                response->set_request_id(request->request_id());
                response->set_success(true);
                return ;
            }
            //3. 统计所有消息中用户ID列表
            std::unordered_set<std::string> user_id_lists; 
            for (const auto &msg : msg_lists) 
            {
                user_id_lists.insert(msg.user_id());
            }
            //4. 从用户子服务进行批量用户信息获取
            std::vector<std::string> user_ids(user_id_lists.begin(), user_id_lists.end());
            std::shared_ptr<brpc::Channel> channel = _service_manager->choose(_user_service_name);
            if(channel==nullptr)
            {
                LOG_ROOT_ERROR<<"用户子服务未找到";
                return errctl("用户子服务未找到");
            }
            UserServiceClient user_client(channel);
            std::vector<UserInfo> user_infos;
            bool ret=user_client.GetMultiUserInfo(user_ids,user_infos);
            if(!ret)
            {
                LOG_ROOT_ERROR<<"用户信息获取失败";
                return errctl("用户信息获取失败");
            }
            std::unordered_map<std::string, UserInfo> user_lists;
            for(int i=0;i<user_ids.size();i++)
            {
                user_lists[user_ids[i]]=user_infos[i];
            }

            //5. 组织响应
            response->set_request_id(request->request_id());
            response->set_success(true);
            for (const auto &msg : msg_lists) 
            {
                auto message_info = response->add_msg_list();
                message_info->set_message_id(msg.message_id());
                message_info->set_chat_session_id(msg.session_id());
                message_info->set_timestamp(boost::posix_time::to_time_t(msg.create_time()));
                message_info->mutable_sender()->CopyFrom(user_lists[msg.user_id()]);
                message_info->mutable_message()->set_message_type(MessageType::STRING);
                message_info->mutable_message()->mutable_string_message()->set_content(msg.content());
            }
        }
    public:
        //消息队列有消息到了的回调函数
        void onMessage(const std::string& content)
        {
        //1. 取出序列化的消息内容，进行反序列化
        LOG_ROOT_DEBUG<<"消息队列有消息到来";
        LOG_ROOT_DEBUG<<content;
        MessageInfo message;
        bool ret=message.ParseFromString(content);
        if(!ret)
        {
            LOG_ROOT_ERROR<<"消息反序列化失败";
            return ;
        }
        MessageType type=message.message().message_type();
        Message msg(message.message_id(),message.chat_session_id(),message.sender().user_id(),type,boost::posix_time::from_time_t(message.timestamp()));
        std::string file_id,file_name;
        std::shared_ptr<brpc::Channel> channel = _service_manager->choose(_file_service_name);
        if(channel==nullptr)
        {
            LOG_ROOT_ERROR<<"文件存储子服务未找到";
            return ;
        }
        FileServiceClient client(channel);
        //2. 根据不同的消息类型进行不同的处理
        //2. 如果是一个图片/语音/文件消息，则取出数据存储到文件子服务中，并获取文件ID
        switch(type) 
        {
            case MessageType::STRING:
            {
                msg._content = message.message().string_message().content();
                bool ret=_es->Insert(msg);
                if(!ret)
                {
                    LOG_ROOT_ERROR<<"es搜索引擎插入失败";
                    return ;
                }
                break;
            }
            case MessageType::IMAGE:
            {
                msg._content = message.message().image_message().image_content();
                bool ret=client.PutSingleFile(" ",msg.content(),file_id);
                if (!ret) 
                {
                    LOG_ROOT_ERROR << "上传图片到文件子服务失败";
                    return;
                }
                break;
            }
            case MessageType::FILE:
            {
                msg._content = message.message().file_message().file_contents();
                file_name=message.message().file_message().file_name();
                bool ret=client.PutSingleFile(file_name,msg.content(),file_id);
                if (!ret) 
                {
                    LOG_ROOT_ERROR << "上传文件到文件子服务失败";
                    return;
                }
                break;
            }
            case MessageType::SPEECH:
            {
                msg._content = message.message().speech_message().file_contents();
                bool ret=client.PutSingleFile(" ",msg.content(),file_id);
                if (!ret) 
                {
                    LOG_ROOT_ERROR << "上传语音到文件子服务失败";
                    return;
                }
                break;
            }
            default:
            {
                LOG_ROOT_ERROR<<"未知的消息类型";
                return ;
            }
        }
        msg._file_id=file_id;
        msg._file_name=file_name;
        msg._file_size=msg.content().size();
        //3. 将消息存储到mysql中
        ret=_mysql->insert(msg);
        if(!ret)
        {
            LOG_ROOT_ERROR<<"mysql存储失败";
            return ;
        }
    }
    private:
        ESMessage::ptr _es;
        MessageTable::ptr _mysql;
        std::string _exchange_name;
        std::string _routing_key;
        ServiceManager::ptr _service_manager;
        MQClient::ptr _mq;
        const std::string _file_service_name="/service/file_service";
        const std::string _user_service_name="/service/user_service";
    };
    class MessageServer
    {
    public:
        using ptr=std::shared_ptr<MessageServer>;
        MessageServer(Registry::ptr reg,std::shared_ptr<brpc::Server> rpc):_reg_client(reg),_rpc_server(rpc)
        {}
        void start()
        {
            _rpc_server->RunUntilAskedToQuit();
        }
    private:
        Registry::ptr _reg_client;
        std::shared_ptr<brpc::Server> _rpc_server;
    };
    class MessageServerBuild
    {
    public:
        void make_reg_object(const std::string& reg_host,const std::string& service_name,const std::string& service_host)
        {
            _reg_client=std::make_shared<Registry>(reg_host);
            _reg_client->registry(service_name,service_host);
        }
        void make_rpc_server(uint16_t port,int timeout,int thread_num)
        {
            if(_reg_client==nullptr)
            {
                LOG_ROOT_ERROR<<"未初始化服务注册模块";
                abort();
            }
            if(_mq==nullptr)
            {
                LOG_ROOT_ERROR<<"未初始化消息队列";
                abort();
            }
            if(_service_manager==nullptr)
            {
                LOG_ROOT_ERROR<<"未初始化服务管理模块";
                abort();
            }
            if(_discover==nullptr)
            {
                LOG_ROOT_ERROR<<"未初始化服务发现模块";
                abort();
            }
            if(_mysql==nullptr)
            {
                LOG_ROOT_ERROR<<"未初始化mysql";
                abort();
            }
            if(_es==nullptr)
            {
                LOG_ROOT_ERROR<<"未初始化es";
                abort();
            }
            _rpc_server=std::make_shared<brpc::Server>();
            MessageServiceImpl* message_service=new MessageServiceImpl(_mysql,_es,_mq,_service_manager,_exchange_name,_routing_key);
            int ret=_rpc_server->AddService(message_service,brpc::ServiceOwnership::SERVER_OWNS_SERVICE);
            if(ret<0)
            {
                LOG_ROOT_ERROR<<"添加rpc服务失败";
                return ;
            }
            brpc::ServerOptions op;
            op.idle_timeout_sec=timeout;
            op.num_threads=thread_num;
            ret=_rpc_server->Start(port,&op);
            if(ret<0)
            {
                LOG_ROOT_ERROR<<"rpc服务启动失败";
                return ;
            }
            auto  cb = std::bind(&MessageServiceImpl::onMessage, message_service, std::placeholders::_1);
            _mq->consume(_routing_key, cb);
        }
        void make_dis_object(const std::string &dis_host, const std::vector<std::string>& dis_dir)
        {
            _service_manager = std::make_shared<ServiceManager>();
            for(auto& dir:dis_dir)
                _service_manager->add_concern(dir);
            auto put_cb = std::bind(&ServiceManager::onServiceOnline, _service_manager.get(), std::placeholders::_1, std::placeholders::_2);
            auto del_cb = std::bind(&ServiceManager::onServiceOffline, _service_manager.get(), std::placeholders::_1, std::placeholders::_2);
            _discover= std::make_shared<Discovery>(dis_host, put_cb, del_cb);
            _discover->discover(dis_dir[0]);
        }
        void make_mq_object(const std::string& user,const std::string& password,const std::string& ip, const std::string& exchange_name ,const std::string& queue_name,const std::string& routing_key)
        {
            _mq=std::make_shared<MQClient>(user,password,ip);
            _exchange_name=exchange_name;
            _routing_key=routing_key;
            _mq->declareComponents(exchange_name,queue_name,routing_key);
        }
        void make_mysql_object(const std::string& host,const std::string& user,const std::string& password,const std::string& db, const std::string& charset, int port, int pool_size)
        {
            _mysql=ODBFactory::create(user,password,host,db,charset,port,pool_size);
        }
        void make_es_object(const std::vector<std::string>& hosts={"http://127.0.0.1:9200/"})
        {
            _es=std::make_shared<elasticlient::Client>(hosts);
        }
        MessageServer::ptr newMessageServer()
        {
            if(_rpc_server==nullptr)
            {
                LOG_ROOT_ERROR<<"未初始化rpc服务器";
                abort();
            }
            MessageServer::ptr ret = std::make_shared<MessageServer>(_reg_client,_rpc_server);
            return  ret;
        }
    private:
        Registry::ptr _reg_client;
        std::shared_ptr<brpc::Server> _rpc_server;
        MQClient::ptr _mq;
        std::shared_ptr<odb::mysql::database> _mysql;
        std::shared_ptr<elasticlient::Client> _es;
        std::string _exchange_name;
        std::string _routing_key;
        ServiceManager::ptr _service_manager;
        Discovery::ptr _discover;
    };
};
