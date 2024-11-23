#pragma once
#include <string>
#include <cstddef> 
#include<memory>
#include <odb/nullable.hxx>
#include <odb/core.hxx>
namespace MindbniM
{
    #pragma db object table("chat_session_member")
    class ChatSessionMember 
    {
    public:
        using ptr=std::shared_ptr<ChatSessionMember>;
        ChatSessionMember()=default;
        ChatSessionMember(const std::string &ssid, const std::string &uid): _session_id(ssid), _user_id(uid)
        {}
    public:
        friend class odb::access;
        #pragma db id auto
        unsigned long _id;                  //主键id
        #pragma db type("varchar(64)") index 
        std::string _session_id;            //会话id
        #pragma db type("varchar(64)") 
        std::string _user_id;               //用户id
    };
}