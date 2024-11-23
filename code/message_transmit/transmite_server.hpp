#pragma once
#include"log.hpp"
#include"etcd.hpp"
#include"channel.hpp"
#include"transmite.pb.h"
#include<brpc/server.h>
#include"mysql_chat_session_member.hpp"
#include"rabbitmq.hpp"
#include"user_service_client.hpp"
namespace MindbniM
{
    class TransmitServiceImpl : public MsgTransmitService
    {
    public:
        TransmitServiceImpl(const ServiceManager::ptr& service_manager,std::shared_ptr<odb::mysql::database> mysql,const MQClient::ptr& mq,const std::string& exchange_name, const std::string &routing_key):_exchange_name(exchange_name),_routing_key(routing_key)
        {
            _service_manager=service_manager;
            _odb=std::make_shared<ChatSessionMemberTable>(mysql);
            _mq=mq;
        }
        void GetTransmitTarget(google::protobuf::RpcController* controller,const NewMessageReq* req,GetTransmitTargetRsp* rsp,google::protobuf::Closure* done)
        {
            brpc::ClosureGuard guard(done);
            auto errctl = [req, rsp](const std::string &err) -> void
            {
                rsp->set_request_id(req->request_id());
                rsp->set_success(false);
                rsp->set_errmsg(err);
            };
            //获取用户信息和该会话的id
            std::string user_id=req->user_id();
            std::string chat_session_id=req->chat_session_id();
            std::shared_ptr<brpc::Channel> channel=_service_manager->choose(_UserServiceName);
            if(channel==nullptr)
            {
                LOG_ROOT_ERROR<<"未找到用户管理子服务";
                return  errctl("没有找到用户管理服务");
            }
            UserServiceClient client(channel);
            UserInfo user;
            bool ret=client.GetUserInfo(user_id,user);
            if(!ret)
            {
                LOG_ROOT_ERROR<<"获取用户信息失败";
                return errctl("获取用户信息失败");
            }
            //用户发布的这条消息的信息
            MessageInfo message;
            message.set_message_id(UUID::Get());
            message.set_chat_session_id(chat_session_id);
            message.set_timestamp(time(nullptr));
            message.mutable_sender()->CopyFrom(user);
            message.mutable_message()->CopyFrom(req->message());
            //获取所有会话成员
            std::vector<ChatSessionMember> users=_odb->get_members(chat_session_id);
            if(users.empty())
            {
                LOG_ROOT_ERROR<<"获取会话成员失败";
                return errctl("获取会话成员失败");
            }
            // 将封装完毕的消息，发布到消息队列，待消息存储子服务进行消息持久化
            ret=_mq->publish(_exchange_name,_routing_key,message.SerializeAsString());
            //返回响应
            rsp->set_request_id(req->request_id());
            rsp->set_success(true);
            rsp->mutable_message()->CopyFrom(message);
            for(auto& id:users)
            {
                rsp->add_target_id_list(id._user_id);
            }
        }
    private:
        const std::string _UserServiceName;
        ServiceManager::ptr _service_manager;
        ChatSessionMemberTable::ptr _odb;
        MQClient::ptr _mq;
        std::string _exchange_name;
        std::string _routing_key;
    };
    class TransmitServer
    {
    public:
        using ptr=std::shared_ptr<TransmitServer>;
        TransmitServer(Registry::ptr reg,std::shared_ptr<brpc::Server> rpc):_reg_client(reg),_rpc_server(rpc)
        {}
        void start()
        {
            _rpc_server->RunUntilAskedToQuit();
        }
    private:
        Registry::ptr _reg_client;
        std::shared_ptr<brpc::Server> _rpc_server;
    };
    class TransmitServerBuild
    {
    public:
        void make_reg_object(const std::string& reg_host,const std::string& service_name,const std::string& service_host)
        {
            _reg_client=std::make_shared<Registry>(reg_host);
            _reg_client->registry(service_name,service_host);
        }
        void make_dis_object(const std::string &dis_host, const std::string dis_dir)
        {
            _service_manager = std::make_shared<ServiceManager>();
            _service_manager->add_concern(dis_dir);
            auto put_cb = std::bind(&ServiceManager::onServiceOnline, _service_manager.get(), std::placeholders::_1, std::placeholders::_2);
            auto del_cb = std::bind(&ServiceManager::onServiceOffline, _service_manager.get(), std::placeholders::_1, std::placeholders::_2);
            _discover= std::make_shared<Discovery>(dis_host, put_cb, del_cb);
            _discover->discover(dis_dir);
        }
        void make_mysql_object(const std::string& host,const std::string& user,const std::string& password,const std::string& db, const std::string& charset, int port, int pool_size)
        {
            _odb=ODBFactory::create(user,password,host,db,charset,port,pool_size);
        }
        void make_mq_object(const std::string& user,const std::string& password,const std::string& ip, const std::string& exchange_name ,const std::string& queue_name,const std::string& routing_key)
        {
            _mq=std::make_shared<MQClient>(user,password,ip);
            _exchange_name=exchange_name;
            _routing_key=routing_key;
            _mq->declareComponents(exchange_name,queue_name,routing_key);
        }
        void make_rpc_server(uint16_t port,int timeout,int thread_num)
        {
            if(_reg_client==nullptr)
            {
                LOG_ROOT_ERROR<<"未初始化服务注册模块";
                abort();
            }
            if(_service_manager==nullptr)
            {
                LOG_ROOT_ERROR<<"未初始化服务管理模块";
                abort();
            }
            if(_odb==nullptr)
            {
                LOG_ROOT_ERROR<<"未初始化数据库模块";
                abort();
            }
            if(_mq==nullptr)
            {
                LOG_ROOT_ERROR<<"未初始化消息队列模块";
                abort();
            }
            _rpc_server=std::make_shared<brpc::Server>();
            TransmitServiceImpl* speech_service=new TransmitServiceImpl(_service_manager,_odb,_mq,_exchange_name,_routing_key);
            int ret=_rpc_server->AddService(speech_service,brpc::ServiceOwnership::SERVER_OWNS_SERVICE);
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
        TransmitServer::ptr newTransmitServer()
        {
            if(_rpc_server==nullptr)
            {
                LOG_ROOT_ERROR<<"未初始化rpc服务器";
                abort();
            }
            return std::make_shared<TransmitServer>(_reg_client,_rpc_server);
        }
    private:
        Registry::ptr _reg_client;
        std::shared_ptr<brpc::Server> _rpc_server;
        ServiceManager::ptr _service_manager;
        Discovery::ptr _discover;
        std::shared_ptr<odb::mysql::database> _odb;
        MQClient::ptr _mq;
        std::string _exchange_name;
        std::string _routing_key;
    };
};