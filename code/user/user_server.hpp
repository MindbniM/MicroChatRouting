#pragma once
#include "log.hpp"
#include "etcd.hpp"
#include "channel.hpp"
#include "user.pb.h"
#include "base.pb.h"
#include <brpc/server.h>
#include "es_data.hpp"
#include "mysql_user.hpp"
#include "redis_data.hpp"
#include "dms.hpp"
namespace MindbniM
{
    class UserServiceImpl : public UserService
    {
    public:
        UserServiceImpl(std::shared_ptr<odb::mysql::database> mysql,std::shared_ptr<sw::redis::Redis> redis,
            std::shared_ptr<elasticlient::Client> es,ServiceManager::ptr _service_manager, DMSClient::ptr dms)
        {
            _usertable = std::make_shared<UserTable>(std::shared_ptr<odb::mysql::database>());
            _esuser = std::make_shared<ESUser>(es);
            _session = std::make_shared<Session>(redis);
            _status = std::make_shared<Status>(redis);
            _codes = std::make_shared<Codes>(redis);
            _service_manager = _service_manager;
            _dms = dms;
        }
        void UserRegisterReq(google::protobuf::RpcController *controller, const UserRegisterReq *req, UserRegisterRsp *rsp, google::protobuf::Closure *done)
        {
            brpc::ClosureGuard guard(done);
        }
        void UserLogin(google::protobuf::RpcController *controller, const UserLoginReq *req, UserLoginRsp *rsp, google::protobuf::Closure *done)
        {
            brpc::ClosureGuard guard(done);
        }
        void GetPhoneVerifyCode(google::protobuf::RpcController *controller, const PhoneVerifyCodeReq *req, PhoneVerifyCodeRsp *rsp, google::protobuf::Closure *done)
        {
            brpc::ClosureGuard guard(done);
        }
        void PhoneRegister(google::protobuf::RpcController *controller, const PhoneRegisterReq *req, PhoneRegisterRsp *rsp, google::protobuf::Closure *done)
        {
            brpc::ClosureGuard guard(done);
        }
        void PhoneLogin(google::protobuf::RpcController *controller, const PhoneLoginReq *req, PhoneLoginRsp *rsp, google::protobuf::Closure *done)
        {
            brpc::ClosureGuard guard(done);
        }
        void GetUserInfo(google::protobuf::RpcController *controller, const GetUserInfoReq *req, GetUserInfoRsp *rsp, google::protobuf::Closure *done)
        {
            brpc::ClosureGuard guard(done);
        }
        void GetMultiUserInfo(google::protobuf::RpcController *controller, const GetMultiUserInfoReq *req, GetMultiUserInfoRsp *rsp, google::protobuf::Closure *done)
        {
            brpc::ClosureGuard guard(done);
        }
        void SetUserAvatar(google::protobuf::RpcController *controller, const SetUserAvatarReq *req, SetUserAvatarRsp *rsp, google::protobuf::Closure *done)
        {
            brpc::ClosureGuard guard(done);
        }
        void SetUserNickname(google::protobuf::RpcController *controller, const SetUserNicknameReq *req, SetUserNicknameRsp *rsp, google::protobuf::Closure *done)
        {
            brpc::ClosureGuard guard(done);
        }
        void SetUserDescription(google::protobuf::RpcController *controller, const SetUserDescriptionReq *req, SetUserDescriptionRsp *rsp, google::protobuf::Closure *done)
        {
            brpc::ClosureGuard guard(done);
        }
        void SetUserPhoneNumber(google::protobuf::RpcController *controller, const SetUserPhoneNumberReq *req, SetUserPhoneNumberRsp *rsp, google::protobuf::Closure *done)
        {
            brpc::ClosureGuard guard(done);
        }

    private:
        UserTable::ptr _usertable;
        ESUser::ptr _esuser;

        Session::ptr _session;
        Status::ptr _status;
        Codes::ptr _codes;

        ServiceManager::ptr _service_manager;

        DMSClient::ptr _dms;
    };

    class UserServer
    {
    public:
        using ptr = std::shared_ptr<UserServer>;
        UserServer(Registry::ptr reg, std::shared_ptr<brpc::Server> rpc) : _reg_client(reg), _rpc_server(rpc)
        {}
        void start()
        {
            _rpc_server->RunUntilAskedToQuit();
        }

    private:
        Registry::ptr _reg_client;
        std::shared_ptr<brpc::Server> _rpc_server;
        //Discovery::ptr _discover;
        //std::shared_ptr<odb::mysql::database> _mysql;
        //std::shared_ptr<sw::redis::Redis> _redis;
        //std::shared_ptr<elasticlient::Client> _es;
    };
    class UserServerBuild
    {
    public:
        void make_reg_object(const std::string &reg_host, const std::string &service_name, const std::string &service_host)
        {
            _reg_client = std::make_shared<Registry>(reg_host);
            _reg_client->registry(service_name, service_host);
        }
        void make_dis_object(const std::string &dis_host, const std::string dis_dir)
        {
            _service_manager = std::make_shared<ServiceManager>();
            _service_manager->add_concern(dis_dir);
            auto put_cb = std::bind(&ServiceManager::onServiceOnline, _service_manager.get(), std::placeholders::_1, std::placeholders::_2);
            auto del_cb = std::bind(&ServiceManager::onServiceOffline, _service_manager.get(), std::placeholders::_1, std::placeholders::_2);
            Discovery::ptr dclient = std::make_shared<Discovery>(dis_host, put_cb, del_cb);
            dclient->discover(dis_dir);
        }
        void make_mysql_object(const std::string &host,int port, const std::string &user, const std::string &passwd, const std::string &db,const std::string& set,int poll_count)
        {
            _mysql=ODBFactory::create(user,passwd,host,db,set,port,poll_count);
        }
        void make_redis_object(const std::string &host, int port, int db)
        {
            _redis = RedisFactory::create(host, port, db);
        }
        void make_es_object(const std::vector<std::string> &hosts)
        {
            _es = std::make_shared<elasticlient::Client>(hosts);
        }
        //rpc服务器应该最后调用
        void make_rpc_server(uint16_t port, int timeout, int thread_num)
        {
            _rpc_server = std::make_shared<brpc::Server>();
            _dms = std::make_shared<DMSClient>();
            if(_mysql == nullptr)
            {
                LOG_ROOT_ERROR << "未初始化mysql数据库";
                abort();
            }
            if(_redis == nullptr)
            {
                LOG_ROOT_ERROR << "未初始化redis数据库";
                abort();
            }
            if(_es == nullptr)
            {
                LOG_ROOT_ERROR << "未初始化es数据库";
                abort();
            }
            if(_service_manager == nullptr)
            {
                LOG_ROOT_ERROR << "未初始化服务发现模块";
                abort();
            }
            if(_dms == nullptr)
            {
                LOG_ROOT_ERROR << "未初始化短信模块";
                abort();
            }
            UserServiceImpl *user_service = new UserServiceImpl(_mysql, _redis, _es, _service_manager, _dms);
            int ret = _rpc_server->AddService(user_service, brpc::ServiceOwnership::SERVER_OWNS_SERVICE);
            if (ret < 0)
            {
                LOG_ROOT_ERROR << "添加rpc服务失败";
                return;
            }
            brpc::ServerOptions op;
            op.idle_timeout_sec = timeout;
            op.num_threads = thread_num;
            ret = _rpc_server->Start(port, &op);
            if (ret < 0)
            {
                LOG_ROOT_ERROR << "rpc服务启动失败";
                return;
            }
        }
        UserServer::ptr newUserServer()
        {
            return std::make_shared<UserServer>(_reg_client, _rpc_server);
        }

    private:
        Registry::ptr _reg_client;
        Discovery::ptr _discover;
        std::shared_ptr<brpc::Server> _rpc_server;
        std::shared_ptr<odb::mysql::database> _mysql;
        std::shared_ptr<sw::redis::Redis> _redis;
        std::shared_ptr<elasticlient::Client> _es;
        ServiceManager::ptr _service_manager;

        DMSClient::ptr _dms;
    };
};
            //UserTable::ptr usertable,
            //ESUser::ptr esuser,
            //Session::ptr session,
            //Status::ptr status,
            //Codes::ptr codes,
            //ServiceManager::ptr _service_manager,
            //DMSClient::ptr dms)