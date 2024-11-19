#pragma once
#include"../odb/user.hxx"
#include"icsearch.hpp"
namespace MindbniM
{
    class ESUser
    {
    public:
        ESUser(const std::vector<std::string>& hosts={"http://127.0.0.1:9200/"}):_client(hosts)
        {}
        ESUser(const std::shared_ptr<elasticlient::Client>& client):_client(client)
        {}
        bool CreateIndex()
        {
            ESClient::CreateValue cv("user");
            cv.append("user_id", "keyword", "standard", true)
                .append("nickname")
                .append("phone", "keyword", "standard", true)
                .append("description", "text", "standard", false)
                .append("avatar_id", "keyword", "standard", false);
            return _client.Create(cv);
        }
        bool Insert(const User& user)
        {
            ESClient::InsertValue iv("user");
            iv.append("user_id", user._user_id);
            if(user._nickname!=nullptr) iv.append("nickname", *user._nickname);
            if(user._phone!=nullptr) iv.append("phone", *user._phone);
            if(user._description!=nullptr) iv.append("description", *user._description);
            if(user._avatar_id!=nullptr) iv.append("avatar_id", *user._avatar_id);
            return _client.Insert(iv,user._user_id);
        }
        std::vector<User> Search(const std::string& key,const std::vector<std::string> &uid_list={})
        {
            ESClient::SearchValue sv("user");
            sv.append_should_match("phone.keyword", key)
                .append_should_match("user_id.keyword", key)
                .append_should_match("nickname", key);
            if(uid_list.size()>0) sv.append_must_not_terms("user_id.keyword", uid_list);
            Json::Value root=_client.Search(sv);
            int sz = root.size();
            LOG_ROOT_DEBUG<<"检索到"<<sz<<"条数据";
            std::vector<User> ret(sz);
            for (int i = 0; i < sz; i++) 
            {
                User user;
                user._user_id=root[i]["_source"]["user_id"].asString();
                user._nickname=root[i]["_source"]["nickname"].asString();
                user._description=root[i]["_source"]["description"].asString();
                user._phone=root[i]["_source"]["phone"].asString();
                user._avatar_id=root[i]["_source"]["avatar_id"].asString();
                ret[i]=std::move(user);
            }
            return ret;
        }
        bool Remove(const std::string& uid)
        {
            ESClient::RemoveValue rv("user","_doc",uid);
            return _client.Remove(rv);
        }
    private:
        ESClient _client;
    };
}