#pragma once
#include <odb/query.hxx>
#include "odb.hpp"
#include "message-odb.hxx"
#include<odb/core.hxx>
#include <odb/result.hxx>
namespace MindbniM
{
    class MessageTable
    {
    public:
        using ptr = std::shared_ptr<MessageTable>;
        MessageTable(const std::shared_ptr<odb::mysql::database> &odb) : _odb(odb)
        {}
        //新增消息
        bool insert(Message& message)
        {
            try
            {
                odb::transaction t(_odb->begin());
                _odb->persist(message);
                t.commit();
            }
            catch (const std::exception &e)
            {
                LOG_ROOT_ERROR << "新增消息失败: " << e.what();
                return false;
            }
            return true;
        }
        //通过消息 ID 获取消息信息
        Message::ptr get_message(const std::string &message_id)
        {
            Message::ptr ret;
            try
            {
                odb::transaction t(_odb->begin());
                ret.reset(_odb->query_one<Message>(odb::query<Message>::message_id == message_id));
                t.commit();
            }
            catch (const std::exception &e)
            {
                LOG_ROOT_ERROR << "通过id获取消息失败: " << e.what();
                return nullptr;
            }
            return ret;
        }
        //通过会话 ID，时间范围，获取指定时间段之内的消息，并按时间进行排序
        std::vector<Message> get_message(const std::string &session_id, const boost::posix_time::ptime &start_time, const boost::posix_time::ptime &end_time)
        {
            std::vector<Message> ret;
            try
            {
                odb::transaction t(_odb->begin());
                odb::result<Message> r = _odb->query<Message>((odb::query<Message>::session_id == session_id 
                    && odb::query<Message>::create_time >= start_time 
                    && odb::query<Message>::create_time <= end_time )+ " ORDER BY create_time ASC");
                for (auto &i : r)
                {
                    ret.emplace_back(std::move(i));
                }
                t.commit();
            }
            catch (const std::exception &e)
            {
                LOG_ROOT_ERROR << "通过会话id,时间范围获取消息失败: " << e.what();
                return ret;
            }
            return ret;
        }
        //通过会话 ID，消息数量，获取最近的 N 条消息（逆序+limit 即可）
        std::vector<Message> get_message(const std::string &session_id, int count)
        {
            std::vector<Message> ret;
            try
            {
                odb::transaction t(_odb->begin());
                odb::result<Message> r = _odb->query<Message>((odb::query<Message>::session_id == session_id) + " ORDER BY create_time DESC LIMIT " + std::to_string(count));
                for (auto &i : r)
                {
                    ret.emplace_back(std::move(i));
                }
                t.commit();
            }
            catch (const std::exception &e)
            {
                LOG_ROOT_ERROR << "通过会话id获取最近消息失败: " << e.what();
                return ret;
            }
            return ret;
        }
    private:
        std::shared_ptr<odb::mysql::database> _odb;
    };
}