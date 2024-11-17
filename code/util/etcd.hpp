#pragma once
#include<etcd/Client.hpp>
#include<etcd/KeepAlive.hpp>
#include<etcd/Response.hpp>
#include<etcd/Value.hpp>
#include<etcd/Watcher.hpp>
#include"log.hpp"
namespace MindbniM
{
    class Registry
    {
    public:
        using ptr=std::shared_ptr<Registry>;
        Registry(const std::string& host):_client(std::make_unique<etcd::Client>(host)),_ka(_client->leasekeepalive(3).get()),_lease_id(_ka->Lease())
        {}
        bool registry(const std::string& key,const std::string& value)
        {
            etcd::Response re=_client->put(key,value,_lease_id).get();
            if(!re.is_ok())
            {
                LOG_ROOT_ERROR<<"服务注册失败"<<":"<<re.error_message();
                return false;
            }
            LOG_ROOT_DEBUG<<"服务注册:"<<key<<" : "<<value;
            return true;
        }
    private:
        std::unique_ptr<etcd::Client> _client;
        std::shared_ptr<etcd::KeepAlive> _ka;
        int64_t _lease_id;
    };
    class Discovery
    {
    public:
        using ptr=std::shared_ptr<Discovery>;
        using CallBack=std::function<void(const std::string&,const std::string&)>;
        Discovery(const std::string& host,const CallBack& put=nullptr,const CallBack& del=nullptr):_client(std::make_unique<etcd::Client>(host)),_put(put),_del(del)
        {}
        bool discover(const std::string& dir)
        {
            etcd::Response re=_client->ls(dir).get();
            if(!re.is_ok())
            {
                LOG_ROOT_ERROR<<"服务发现错误"<<re.error_message();
                return false;
            }
            int n=re.keys().size();
            if(_put!=nullptr)
            {
                for(int i=0;i<n;i++)
                {
                    _put(re.key(i),re.value(i).as_string());
                }
            }
            _watch=std::make_unique<etcd::Watcher>(*_client,dir,std::bind(&Discovery::_CallBack,this,std::placeholders::_1),true);
            return true;
        }
        bool wait()
        {
            return _watch->Wait();
        }
    private:
        void _CallBack(const etcd::Response& re)
        {
            if(!re.is_ok())
            {
                LOG_ROOT_ERROR<<re.error_message();
                return;
            }
            for(const auto& ev: re.events())
            {
                if(ev.event_type()==etcd::Event::EventType::PUT)
                {
                    LOG_ROOT_INFO<<"新增服务: "<<ev.kv().key()<<" : "<<ev.kv().as_string();
                }
                if(ev.event_type()==etcd::Event::EventType::DELETE_)
                {
                    LOG_ROOT_INFO<<"服务下线: "<<ev.prev_kv().key()<<" : "<<ev.prev_kv().as_string();
                }
            }
        }
        CallBack _put;
        CallBack _del;
        std::unique_ptr<etcd::Client> _client;
        std::unique_ptr<etcd::Watcher> _watch;
    };
}
