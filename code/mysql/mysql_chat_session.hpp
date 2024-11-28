#pragma once
#include <odb/query.hxx>
#include "odb.hpp"
#include "chat_session-odb.hxx"
#include"chat_session_member-odb.hxx"
#include<odb/core.hxx>
#include <odb/result.hxx>
namespace MindbniM
{
    class ChatSessionTable
    {
    public:
        using ptr=std::shared_ptr<ChatSessionTable>;
        ChatSessionTable(const std::shared_ptr<odb::mysql::database> &odb) : _odb(odb)
        {}
        //新增会话信息
        bool Insert(ChatSession& cs)
        {
            try
            {
                odb::transaction t(_odb->begin());
                _odb->persist(cs);
                t.commit();
            }
            catch (const std::exception &e)
            {
                LOG_ROOT_ERROR << "新增会话信息失败: " << e.what();
                return false;
            }
            return true;
        }
        //删除会话信息
        bool Remove(const std::string& ssid)
        {
            try
            {
                odb::transaction t(_odb->begin());
                _odb->erase<ChatSession>(odb::query<ChatSession>::chat_session_id == ssid);
                _odb->erase<ChatSessionMember>(odb::query<ChatSessionMember>::session_id == ssid);
                t.commit();
            }
            catch (const std::exception &e)
            {
                LOG_ROOT_ERROR << "删除会话信息失败: " << e.what();
                return false;
            }
            return true;
        }
        bool remove(const std::string &uid, const std::string &pid) 
        {
            try 
            {
                odb::transaction t(_odb->begin());
                odb::result<SingleChatSession> res = _odb->query_one<SingleChatSession>(
                    odb::query<SingleChatSession>::csm1::user_id == uid && 
                    odb::query<SingleChatSession>::csm2::user_id == pid && 
                    odb::query<SingleChatSession>::css::chat_session_type == ChatSessionType::SINGLE);

                std::string cssid = res->chat_session_id;
                _odb->erase_query<ChatSession>(odb::query<ChatSession>::chat_session_id == cssid);
                _odb->erase_query<ChatSessionMember>(odb::query<ChatSessionMember>::session_id == cssid);
                t.commit();
            }
            catch (std::exception &e) 
            {
                LOG_ROOT_ERROR<<"删除会话失败"<<e.what();
                return false;
            }
            return true;
        }
        //获取指定会话id的会话信息
        ChatSession::ptr GetChatSession(const std::string& ssid)
        {
            ChatSession::ptr ret;
            try
            {
                odb::transaction t(_odb->begin());
                ret=_odb->query_one<ChatSession>(odb::query<ChatSession>::chat_session_id == ssid);
                t.commit();
            }
            catch (const std::exception &e)
            {
                LOG_ROOT_ERROR << "获取会话信息失败: " << e.what();
            }
            return ret;
        }

        //获取指定用户的单聊会话信息
        std::vector<SingleChatSession> GetSingChatSession(const std::string& uid)
        {
            std::vector<SingleChatSession> ret;
            try
            {
                odb::transaction t(_odb->begin());
                odb::result<SingleChatSession> r(_odb->query<SingleChatSession>( odb::query<SingleChatSession>::css::chat_session_type == ChatSessionType::SINGLE && 
                        odb::query<SingleChatSession>::csm1::user_id == uid && 
                        odb::query<SingleChatSession>::csm2::user_id != odb::query<SingleChatSession>::csm1::user_id));
                for(auto& i:r)
                {
                    ret.emplace_back(i);
                }
                t.commit();
            }
            catch (const std::exception &e)
            {
                LOG_ROOT_ERROR << "获取单聊会话信息失败: " << e.what();
            }
            return ret;
        }
        //获取指定用户的群聊会话信息
        std::vector<GroupChatSession> GetGroupChatSession(const std::string& uid)
        {
            std::vector<GroupChatSession> ret;
            try
            {
                odb::transaction t(_odb->begin());
                odb::result<GroupChatSession> r(_odb->query<GroupChatSession>( odb::query<GroupChatSession>::css::chat_session_type == ChatSessionType::GROUP && 
                        odb::query<GroupChatSession>::csm::user_id == uid));
                for(auto& i:r)
                {
                    ret.emplace_back(i);
                }
                t.commit();
            }
            catch (const std::exception &e)
            {
                LOG_ROOT_ERROR << "获取群聊会话信息失败: " << e.what();
            }
            return ret;

        }
    private:
        std::shared_ptr<odb::mysql::database> _odb;
    };
}