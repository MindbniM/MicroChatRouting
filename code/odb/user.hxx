#pragma once
#include <string>
#include <cstddef> 
#include<memory>
#include <odb/nullable.hxx>
#include <odb/core.hxx>
namespace MindbniM
{
    #pragma db object table("user")
    class User
    {
        friend class odb::access;
    public:
        using ptr=std::shared_ptr<User>;
        User()=default;
        User(const std::string& uid,const std::string& nickname,const std::string& password):_user_id(uid),_nickname(nickname),_password(password)
        {}
        User(const std::string& uid,const std::string& phone):_nickname(uid),_user_id(uid),_phone(phone)
        {}
    public:
        #pragma db id auto
        unsigned long _id;                          //主键id
        #pragma db type("varchar(64)") index unique
        std::string _user_id;                       //用户id
        #pragma db type("varchar(64)") index unique
        odb::nullable<std::string> _nickname;       //用户名称 
        odb::nullable<std::string> _description;    //用户签名
        #pragma db type("varchar(64)")
        odb::nullable<std::string> _password;       //密码
        #pragma db type("varchar(64)") index unique
        odb::nullable<std::string> _phone;          //手机号
        #pragma db type("varchar(64)")
        odb::nullable<std::string> _avatar_id;      //用户头像文件id
    };
}