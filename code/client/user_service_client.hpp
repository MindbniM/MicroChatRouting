#pragma once
#include "etcd.hpp"
#include "channel.hpp"
#include "user.pb.h"
#include"uuid.hpp"
namespace MindbniM
{
    class UserServiceClient
    {
    public:
        UserServiceClient(std::shared_ptr<brpc::Channel> channel) : _channel(channel)
        {}
        bool GetUserInfo(const std::string& user_id,UserInfo& ret)
        {
            UserService_Stub stub(_channel.get());
            brpc::Controller cntl;
            GetUserInfoReq req;
            GetUserInfoRsp rsp;
            req.set_request_id(UUID::Get());
            req.set_user_id(user_id);
            stub.GetUserInfo(&cntl,&req,&rsp,nullptr);
            if(cntl.Failed())
            {
                LOG_ROOT_ERROR<<"调用用户管理子服务失败"<<cntl.ErrorText();
                return false;
            }
            if(!rsp.success())
            {
                LOG_ROOT_ERROR<<"调用用户管理子服务失败"<<rsp.errmsg();
                return false;
            }
            ret=rsp.user_info();
            return true;
        }
        bool UserRegister(const std::string& nickname,const std::string&password)
        {
            UserService_Stub stub(_channel.get());
            brpc::Controller cntl;
            UserRegisterReq req;
            UserRegisterRsp rsp;
            req.set_request_id(UUID::Get());
            req.set_nickname(nickname);
            req.set_password(password);
            stub.UserRegister(&cntl,&req,&rsp,nullptr);
            if(cntl.Failed())
            {
                LOG_ROOT_ERROR<<"调用用户管理子服务失败"<<cntl.ErrorText();
                return false;
            }
            if(!rsp.success())
            {
                LOG_ROOT_ERROR<<"调用用户管理子服务失败"<<rsp.errmsg();
                return false;
            }
            return true;
        }
    private:
        std::shared_ptr<brpc::Channel> _channel;
    };
}