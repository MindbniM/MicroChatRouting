#pragma once
#include"../odb/message.hxx"
#include"../util/icsearch.hpp"
namespace MindbniM
{
    class ESMessage
    {
    public:
        using ptr=std::shared_ptr<ESMessage>;
        ESMessage(const std::vector<std::string>& hosts={"http://127.0.0.1:9200/"}):_client(hosts)
        {}
        ESMessage(const std::shared_ptr<elasticlient::Client>& client):_client(client)
        {}
        bool CreateIndex()
        {
            ESClient::CreateValue cv("message");
            cv.append("message") .append("user_id", "keyword", "standard", false) .append("message_id", "keyword", "standard", false)
                .append("create_time", "long", "standard", false) .append("chat_session_id", "keyword", "standard", true)
                .append("content");
            bool ret=_client.Create(cv);
            if (ret == false) 
            {
                LOG_ROOT_ERROR<<"消息信息索引创建失败!";
                return false;
            }
            LOG_ROOT_DEBUG<<"消息信息索引创建成功!";
            return true;
        }
        bool Insert(const Message& message)
        {
            ESClient::InsertValue iv("message");
            iv.append("message_id", message.message_id()) .append("user_id", message.user_id())
                .append("create_time", boost::posix_time::to_time_t(message.create_time())) .append("chat_session_id", message.session_id())
                .append("content", message.content());
            return _client.Insert(iv, message.message_id());
        }
        std::vector<Message> Search(const std::string& key,const std::string& ssid)
        {
            std::vector<Message> ret;
            ESClient::SearchValue sv("message");
            sv.append_must_term("chat_session_id.keyword", ssid).append_must_match("content", key);
            Json::Value res=_client.Search(sv);
            int sz=res.size();
            for(int i=0;i<sz;i++)
            {
                std::string mid=res[i]["_source"]["message_id"].asString();
                std::string uid=res[i]["_source"]["user_id"].asString();
                boost::posix_time::ptime ctime=boost::posix_time::from_time_t(res[i]["_source"]["create_time"].asInt());
                std::string session_id=res[i]["_source"]["chat_session_id"].asString();
                std::string content=res[i]["_source"]["content"].asString();
                ret.emplace_back(mid,ssid,uid,0,ctime);
                ret[i]._content=content;
            }
            return ret;
        }

        bool Remove(const std::string& mid)
        {
            ESClient::RemoveValue rv("message","_doc",mid);
            return _client.Remove(rv);
        }
    private:
        ESClient _client;
    };
}