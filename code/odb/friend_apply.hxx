#pragma once
#include <string>
#include<memory>
#include <cstddef>
#include <odb/core.hxx>

namespace MindbniM
{
#pragma db object table("friend_apply")
    class FriendApply
    {
    public:
        using ptr=std::shared_ptr<FriendApply>;
        FriendApply() {}
        FriendApply(const std::string &uid, const std::string &pid) : _user_id(uid), _peer_id(pid)
        {}


        std::string user_id() const { return _user_id; }
        void user_id(std::string &uid) { _user_id = uid; }

        std::string peer_id() const { return _peer_id; }
        void peer_id(std::string &uid) { _peer_id = uid; }

    public:
        friend class odb::access;
#pragma db id auto
        unsigned long _id;                  // 主键ID
#pragma db type("varchar(64)") index
        std::string _user_id;               //用户ID
#pragma db type("varchar(64)") index
        std::string _peer_id;               //好友ID
    };
}