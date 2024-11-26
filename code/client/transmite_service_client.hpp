#pragma once
#include"log.hpp"
#include"uuid.hpp"
#include"channel.hpp"
#include"transmite.pb.h"
namespace MindbniM
{
    class TransmiteServiceClient
    {
    public:
        using ptr=std::shared_ptr<TransmiteServiceClient>;
        TransmiteServiceClient(std::shared_ptr<brpc::Channel> channel) : _channel(channel)
        {}
        bool GetTransmitTarget(const std::string& user_id,const std::string &ssid,const MessageContent& content
            ,MessageInfo& msg,std::vector<std::string>& user_ids)
        {
            MsgTransmitService_Stub stub(_channel.get());
            brpc::Controller cntl;
            NewMessageReq req;
            GetTransmitTargetRsp rsp;
            req.set_request_id(UUID::Get());
            req.set_user_id(user_id);
            req.set_chat_session_id(ssid);
            req.mutable_message()->CopyFrom(content);
            stub.GetTransmitTarget(&cntl,&req,&rsp,nullptr);
            if(cntl.Failed())
            {
                LOG_ROOT_ERROR<<"调用转发子服务失败"<<cntl.ErrorText();
                return false;
            }
            if(!rsp.success())
            {
                LOG_ROOT_ERROR<<"调用转发子服务失败"<<rsp.errmsg();
                return false;
            }
            msg=rsp.message();
            for(int i=0;i<rsp.target_id_list_size();i++)
            {
                user_ids.push_back(rsp.target_id_list(i));
            }
            return true; 
        }
    private:
        std::shared_ptr<brpc::Channel> _channel;
    };
}