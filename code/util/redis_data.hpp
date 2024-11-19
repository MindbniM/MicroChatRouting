#pragma once
#include<sw/redis++/redis++.h>
#include"log.hpp"
namespace MindbniM
{
    class RedisFactory
    {
    public:
        /*
        * @param host redis服务器地址
        * @param port redis服务器端口
        * @param db 数据库
        * @param keep_alive 是否长连接
        * @return std::shared_ptr<sw::redis::Redis> 返回一个redis连接实例
        */
        std::shared_ptr<sw::redis::Redis> static create(const std::string& host,int port,int db,bool keep_alive=true)
        {
            sw::redis::ConnectionOptions op;
            op.host=host;
            op.port=port;
            op.db=db;
            op.keep_alive=keep_alive;
            return std::make_shared<sw::redis::Redis>(op);
        }
    };
    class Session
    {
    public:
        using ptr=std::shared_ptr<Session>;
        Session(std::shared_ptr<sw::redis::Redis> redis):_redis(redis){}
        void append(const std::string& ssid,const std::string& uid)
        {
            _redis->set(ssid,uid);
        }
        void remove(const std::string& ssid)
        {
            _redis->del(ssid);
        }
        sw::redis::OptionalString get(const std::string& ssid)
        {
            return _redis->get(ssid);
        }
    private:
        std::shared_ptr<sw::redis::Redis> _redis;
    };
    class Status
    {
    public:
        using ptr=std::shared_ptr<Status>;
        Status(std::shared_ptr<sw::redis::Redis> redis):_redis(redis){}
        void append(const std::string& uid,const std::string& status="default")
        {
            _redis->set(uid,status);
        }
        void remove(const std::string& uid)
        {
            _redis->del(uid);
        }
        bool exist(const std::string& uid)
        {
            return _redis->exists(uid);
        }
    private:
        std::shared_ptr<sw::redis::Redis> _redis;
    };
    class Codes
    {
    public:
        using ptr=std::shared_ptr<Codes>;
        Codes(std::shared_ptr<sw::redis::Redis> redis):_redis(redis){}
        void append(const std::string& uid,const std::string& code,const std::chrono::milliseconds& timeout=std::chrono::milliseconds(1000*60))
        {
            _redis->set(uid,code,timeout);
        }
        void remove(const std::string& uid)
        {
            _redis->del(uid);
        }
        sw::redis::OptionalString get(const std::string& uid)
        {
            return _redis->get(uid);
        }
    private:
        std::shared_ptr<sw::redis::Redis> _redis;
    };
}