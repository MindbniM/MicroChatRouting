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
        }
        void GetRecentMsg(google::protobuf::RpcController* controller,const GetRecentMsgReq* request,GetRecentMsgRsp* response,google::protobuf::Closure* done)
        {
            brpc::ClosureGuard guard(done);
        }
        void MsgSearch(google::protobuf::RpcController* controller,const MsgSearchReq* request,MsgSearchRsp* response,google::protobuf::Closure* done)
        {
            brpc::ClosureGuard guard(done);
        }
    private:
        //消息队列有消息到了的回调函数
        void onMessage(const char* body,int sz)
        {
        //1. 取出序列化的消息内容，进行反序列化
        std::string content(body,sz);
        MessageInfo message;
        int ret=message.ParseFromString(content);
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
                bool ret=_es->insert(msg);
                if(!ret)
                {
                    LOG_ROOT_ERROR<<"es搜索引擎插入失败";
                    return ;
                }
                break;
            }
            case MessageType::IMAGE:
            {
                msg._content = message.image_message().image_content();
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
                msg._content = message.file_message().file_contents();
                file_name=message.file_message().file_name();
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
                msg._content = message.speech_message().file_contents();
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
        msg.file_id(file_id);
        msg.file_name(file_name);
        msg.file_size(msg.content().size());
        //3. 将消息存储到mysql中
        bool ret=_mysql->insert(msg);
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
        }
        void make_dis_object(const std::string &dis_host, const std::vector<std::string>& dis_dir)
        {
            _service_manager = std::make_shared<ServiceManager>();
            for(auto& dir:dis_dir)
                _service_manager->add_concern(dir);
            auto put_cb = std::bind(&ServiceManager::onServiceOnline, _service_manager.get(), std::placeholders::_1, std::placeholders::_2);
            auto del_cb = std::bind(&ServiceManager::onServiceOffline, _service_manager.get(), std::placeholders::_1, std::placeholders::_2);
            _discover= std::make_shared<Discovery>(dis_host, put_cb, del_cb);
            for(auto& dir:dis_dir)
                _discover->discover(dir);
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
            return std::make_shared<MessageServer>(_reg_client,_rpc_server);
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