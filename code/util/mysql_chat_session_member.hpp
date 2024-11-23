#pragma once
#include <odb/query.hxx>
#include "odb.hpp"
#include "chat_session_member-odb.hxx"
#include<odb/core.hxx>
#include <odb/result.hxx>
namespace MindbniM
{
    class ChatSessionMemberTable
    {
    public:
        using ptr=std::shared_ptr<ChatSessionMemberTable>;
        ChatSessionMemberTable(const std::shared_ptr<odb::mysql::database>& odb):_odb(odb)
        {}
        bool insert(ChatSessionMember& member)
        {
            try
            {
                odb::transaction tran(_odb->begin());
                _odb->persist(member);
                tran.commit();
            }
            catch(const std::exception& e)
            {
                LOG_ROOT_ERROR<<"插入单聊天会话成员失败: "<<e.what();
                return false;
            }
            return true;
        }
        bool insert(std::vector<ChatSessionMember>& member_list)
        {
            try
            {
                odb::transaction tran(_odb->begin());
                for(auto& member:member_list)
                {
                    _odb->persist(member);
                }
                tran.commit();
            }
            catch(const std::exception& e)
            {
                LOG_ROOT_ERROR<<"插入多个单聊天会话成员失败: "<<e.what();
                return false;
            }
            return true;
        }
        bool erase(ChatSessionMember& member)
        {
            try
            {
                odb::transaction tran(_odb->begin());
                _odb->erase_query<ChatSessionMember>(odb::query<ChatSessionMember>::session_id==member._session_id && odb::query<ChatSessionMember>::user_id==member._user_id);
                tran.commit();
            }
            catch(const std::exception& e)
            {
                LOG_ROOT_ERROR<<"删除单聊天会话成员失败: "<<e.what();
                return false;
            }
            return true;
        }
        bool erase(const std::string& session_id)
        {
            try
            {
                odb::transaction tran(_odb->begin());
                _odb->erase_query<ChatSessionMember>(odb::query<ChatSessionMember>::session_id==session_id);
                tran.commit();
            }
            catch(const std::exception& e)
            {
                LOG_ROOT_ERROR<<"删除整个会话失败: "<<e.what();
                return false;
            }
            return true;
        }
        std::vector<ChatSessionMember> get_members(const std::string& session_id)
        {
            std::vector<ChatSessionMember> ret;
            try
            {
                odb::transaction tran(_odb->begin());
                odb::result<ChatSessionMember> temp=_odb->query<ChatSessionMember>(odb::query<ChatSessionMember>::session_id==session_id);
                for(auto& member:temp)
                {
                    ret.push_back(member);
                }
                tran.commit();
            }
            catch(const std::exception& e)
            {
                LOG_ROOT_ERROR<<"获取会话成员失败: "<<e.what();
                return {};
            }
            return ret;
        }
    private:
        std::shared_ptr<odb::mysql::database> _odb;
    };
}