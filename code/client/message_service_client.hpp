#pragma once
#include"log.hpp"
#include"uuid.hpp"
#include"channel.hpp"
#include"message.pb.h"
namespace MindbniM
{
    class MessageServiceClient
    {
    public:
        MessageServiceClient(std::shared_ptr<brpc::Channel> channel) : _channel(channel)
        {}
        bool GetRecentMsg(const std::string& ssid,int count,std::vector<MessageInfo>& msg)
        {
            brpc::Controller cntl;
            GetRecentMsgReq req;
            GetRecentMsgRsp rsp;
            req.set_session_id(ssid);
            req.set_msg_count(count);
            req.set_request_id(UUID::Get());
            MsgStorageService_Stub stub(_channel.get());
            stub.GetRecentMsg(&cntl,&req,&rsp,nullptr);
            if(cntl.Failed())
            {
                LOG_ROOT_ERROR<<"请求消息存储子服务失败 err:"<<cntl.ErrorText();
                return false;
            }
            if(!rsp.success())
            {
                LOG_ROOT_ERROR<<"请求前N条信息失败 err:"<<rsp.errmsg();
                return false;
            }
            for(int i=0;i<rsp.msg_list_size();i++)
            {
                msg.emplace_back(rsp.msg_list(i));
            }
            return true;
        }
    private:
        std::shared_ptr<brpc::Channel> _channel;
    };
}