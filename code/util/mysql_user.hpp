#pragma once
#include <odb/query.hxx>
#include "odb.hpp"
#include "user.hxx"
#include "user-odb.hxx"
#include<odb/core.hxx>
#include <odb/result.hxx>
namespace MindbniM
{
    class UserTable
    {
    public:
        using ptr = std::shared_ptr<UserTable>;
        UserTable(const std::shared_ptr<odb::mysql::database> &odb) : _odb(odb)
        {}
        bool insert(MindbniM::User& user)
        {
            try
            {
                odb::transaction trans(_odb->begin()); // 启用odb事务操作
                _odb->persist(user);
                trans.commit();
            }
            catch (const std::exception &e)
            {
                LOG_ROOT_ERROR<< "添加用户数据错误: " << e.what();
                return false;
            }
            return true;
        }
        User::ptr select_by_nickname(const std::string &nickname)
        {
            User::ptr ret;
            try
            {
                odb::transaction trans(_odb->begin()); 
                ret.reset(_odb->query_one<User>(odb::query<MindbniM::User>::nickname == nickname));
                trans.commit();
            }
            catch (const std::exception &e)
            {
                LOG_ROOT_ERROR<< "查询用户名错误: " << e.what();
                return nullptr;
            }
            return ret;
        }
        User::ptr select_by_phone(const std::string& phone)
        {
            User::ptr ret;
            try
            {
                odb::transaction trans(_odb->begin()); 
                ret.reset(_odb->query_one<User>(odb::query<MindbniM::User>::phone == phone));
                trans.commit();
            }
            catch (const std::exception &e)
            {
                LOG_ROOT_ERROR<< "查询手机号错误: " << e.what();
                return nullptr;
            }
            return ret;
        }
        User::ptr select_by_id(const std::string& id)
        {
            User::ptr ret;
            try
            {
                odb::transaction trans(_odb->begin()); 
                ret.reset(_odb->query_one<User>(odb::query<MindbniM::User>::user_id == id));
                trans.commit();
            }
            catch (const std::exception &e)
            {
                LOG_ROOT_ERROR<< "查询用户id错误: " << e.what();
                return nullptr;
            }
            return ret;
        }
        std::vector<User> select_multi_users(const std::vector<std::string>& ids)
        {
            std::vector<User> ret;
            try
            {
                odb::transaction trans(_odb->begin());
                odb::result<User> temp=_odb->query<User>(odb::query<MindbniM::User>::user_id.in_range(ids.begin(),ids.end()));
                for(auto &i:temp)
                {
                    ret.push_back(i);
                }
                trans.commit();
            }
            catch(const std::exception& e)
            {
                LOG_ROOT_ERROR<< "查询多个用户错误: " << e.what();
                return {};
            }
            return ret;
        }

        bool update(const User::ptr &user)
        {
            try
            {
                odb::transaction trans(_odb->begin()); // 启用odb事务操作
                _odb->update(*user);
                trans.commit();
            }
            catch (const std::exception &e)
            {
                LOG_ROOT_ERROR<< "更新用户数据错误: " << e.what();
                return false;
            }
            return true;
        }
    private:
        std::shared_ptr<odb::mysql::database> _odb;
    };
}