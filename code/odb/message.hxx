#pragma once
#include <string>
#include <cstddef> 
#include <odb/nullable.hxx>
#include <odb/core.hxx>
#include<memory>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace MindbniM
{
    #pragma db object table("message")
    class Message 
    {
        public:
            using ptr=std::shared_ptr<Message>;
            Message(){}
            Message(const std::string &mid, const std::string &ssid, const std::string &uid,
                const unsigned char mtype,const boost::posix_time::ptime& ctime): _message_id(mid), _session_id(ssid),
                _user_id(uid), _message_type(mtype), _create_time(ctime)
            {}

            std::string message_id() const { return _message_id; }
            std::string session_id() const { return _session_id; }
            std::string user_id() const { return _user_id; }
            unsigned char message_type() const { return _message_type; }
            boost::posix_time::ptime create_time() const { return _create_time; }
            std::string content() const { return _content==nullptr?"":*_content; }


            std::string file_id() const {return _file_id==nullptr?"":*_file_id; }
            std::string file_name() const { return _file_name==nullptr?"":*_file_name; }
            unsigned int file_size() const { return _file_size==nullptr?0:*_file_size; }
        public:
            friend class odb::access;

            #pragma db id auto
            unsigned long _id;                      //主键id

            #pragma db type("varchar(64)") index unique
            std::string _message_id;                //消息ID

            #pragma db type("varchar(64)") index
            std::string _session_id;                //所属会话ID

            #pragma db type("varchar(64)")
            std::string _user_id;                   //发送者用户ID
            unsigned char _message_type;            //消息类型 0-文本；1-图片；2-文件；3-语音

            #pragma db type("TIMESTAMP")
            boost::posix_time::ptime _create_time;  //消息的产生时间

            odb::nullable<std::string> _content;    //文本消息内容--非文本消息可以忽略

            #pragma db type("varchar(64)")
            odb::nullable<std::string> _file_id;    //文件消息的文件ID -- 文本消息忽略

            #pragma db type("varchar(128)")
            odb::nullable<std::string> _file_name;  //文件消息的文件名称 -- 只针对文件消息有效
            odb::nullable<unsigned int> _file_size; //文件消息的文件大小 -- 只针对文件消息有效
    };
    //odb -d mysql --std c++11 --generate-query --generate-schema --profile boost/date-time person.hxx
}