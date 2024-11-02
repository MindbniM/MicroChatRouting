#pragma once
#include"../log/log.hpp"
#include"brpc/channel.h"
namespace MindbniM
{
    using ChannelPtr=std::shared_ptr<brpc::Channel>;
    class ServiceChannel
    {
    public:
        using ptr=std::shared_ptr<ServiceChannel>;
        ServiceChannel(const std::string& ServiceName):_name(ServiceName)
        {}
        bool append(const std::string& host)
        {
            ChannelPtr ch=std::make_shared<brpc::Channel>();
            brpc::ChannelOptions op;
            op.connect_timeout_ms=-1;
            op.timeout_ms=-1;
            op.max_retry=3;
            op.protocol="baidu_std";
            int ret=ch->Init(host.c_str(),&op);
            if(ret<0)
            {
                LOG_ROOT_ERROR<<"初始化信道失败 host: "<<host;
                return false;
            }
            std::unique_lock<std::mutex> lock(_mutex);
            _channels.emplace_back(ch);
            _map[host]=ch;
            return true;
        }
        bool remove(const std::string& host)
        {
            std::unique_lock<std::mutex> lock(_mutex);
            auto it=_map.find(host);
            if(it==_map.end())
            {
                LOG_ROOT_WARNING<<"没有找到该要删除的信道信息 host: "<<host;
                return false;
            }
            auto cit=_channels.begin();
            while(cit!=_channels.end())
            {
                if(*cit==it->second)
                {
                    _channels.erase(cit);
                    break;
                }
                ++cit;
            }
            _map.erase(host);
            return true;
        }
        ChannelPtr choose()
        {
            std::unique_lock<std::mutex> lock(_mutex);
            _index%=_channels.size();
            return _channels[_index++];
        }
    private:
        std::mutex _mutex;
        std::string _name;
        int _index=0;
        std::vector<ChannelPtr> _channels;
        std::unordered_map<std::string,ChannelPtr> _map;
    };
    class ServiceManager
    {
    public:
        ChannelPtr choose(const std::string& ServiceName)     
        {
            std::unique_lock<std::mutex> lock(_mutex);
            if(!_services.count(ServiceName))
            {
                LOG_ROOT_ERROR<<ServiceName<<" 没有能提供该服务的主机";
                return nullptr;
            }
            return _services[ServiceName]->choose();
        }
        void add_concern(const std::string& ServiceName)
        {
            std::unique_lock<std::mutex> lock(_mutex);
            _concern.insert(ServiceName);
        }
        void onServiceOnline(const std::string& ServiceInstance,const std::string& Host)
        {
            std::string ServiceName=getServiceName(ServiceInstance);
            std::unique_lock<std::mutex> lock(_mutex);
            if(!_concern.count(ServiceName))
            {
                LOG_ROOT_DEBUG<<ServiceName<<"服务上线 Host:"<<Host<<"(不关心)";
                return ;       
            }
            if(!_services.count(ServiceName))
            {
                _services[ServiceName]=std::make_shared<ServiceChannel>(ServiceName);
            }
            _services[ServiceName]->append(Host);
        }
        void onServiceOffline(const std::string& ServiceInstance,const std::string& Host)
        {
            std::string ServiceName=getServiceName(ServiceInstance);
            std::unique_lock<std::mutex> lock(_mutex);
            if(!_concern.count(ServiceName))
            {
                LOG_ROOT_DEBUG<<ServiceName<<"服务下线 Host:"<<Host<<"(不关心)";
                return ;       
            }
            if(!_services.count(ServiceName))
            {
                LOG_ROOT_WARNING<<ServiceName<<"要删除的服务不存在";
            }
            _services[ServiceName]->remove(Host);
        }
    private:
        std::string getServiceName(const std::string& ServiceInstance)
        {
            auto pos=ServiceInstance.find_last_of('/');
            if(pos==std::string::npos) return ServiceInstance;
            return ServiceInstance.substr(0,pos);
        }
        std::mutex _mutex;
        std::unordered_set<std::string> _concern;
        std::unordered_map<std::string,ServiceChannel::ptr> _services;
    };
    
}
