#pragma once
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include"log.hpp"
namespace MindbniM
{
    using Server_t = websocketpp::server<websocketpp::config::asio>;
    class Connection
    {
    public:
        struct ConnectionInfo
        {
            std::string uid;
            std::string ssid;
        };
        bool insert(Server_t::connection_ptr con,const std::string &uid,const std::string& ssid)
        {
            std::unique_lock<std::mutex> lock(_mutex);
            _connections[uid] = con;
            _connectionInfos[con] = {uid,ssid};
            LOG_ROOT_DEBUG<<"新增长连接信息";
        }
        Server_t::connection_ptr connection(const std::string& uid)
        {
            std::unique_lock<std::mutex> lock(_mutex);
            auto it=_connections.find(uid);
            if(it!=_connections.end())
            {
                return it->second;
            }
            LOG_ROOT_ERROR<<"未找到长连接信息";
            return nullptr;
        }
        bool getinfo(Server_t::connection_ptr con,ConnectionInfo& info)
        {
            std::unique_lock<std::mutex> lock(_mutex);
            auto it=_connectionInfos.find(con);
            if(it!=_connectionInfos.end())
            {
                info=it->second;
                return true;
            }
            LOG_ROOT_ERROR<<"未找到长连接信息";
            return false;
        }
        bool erase(Server_t::connection_ptr con)
        {
            std::unique_lock<std::mutex> lock(_mutex);
            auto it=_connectionInfos.find(con);
            if(it!=_connectionInfos.end())
            {
                _connections.erase(it->second.uid);
                _connectionInfos.erase(it);
                return true;
            }
            LOG_ROOT_ERROR<<"未找到长连接信息";
            return false;
        }
    private:
        std::unordered_map<std::string, Server_t::connection_ptr> _connections;
        std::unordered_map<Server_t::connection_ptr,ConnectionInfo> _connectionInfos;
        std::mutex _mutex;
    };
}