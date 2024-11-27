#pragma once
#include <odb/query.hxx>
#include "odb.hpp"
#include "relation-odb.hxx"
#include<odb/core.hxx>
#include <odb/result.hxx>
namespace MindbniM
{
    class RelationTable
    {
    public:
        using ptr=std::shared_ptr<RelationTable>;
        RelationTable(const std::shared_ptr<odb::mysql::database> &odb) : _odb(odb)
        {}
        //新增关系
        bool insert(const std::string& uid1,const std::string& uid2)
        {
            try
            {
                Relation r1(uid1,uid2);
                Relation r2(uid2,uid1);
                odb::transaction t(_odb->begin());
                _odb->persist(r1);
                _odb->persist(r2);
                t.commit();
            }
            catch (const std::exception &e)
            {
                LOG_ROOT_ERROR << "新增关系失败: " << e.what();
                return false;
            }
            return true;
        }
        //删除关系
        bool remove(const std::string& uid1,const std::string& uid2)
        {
            try
            {
                odb::transaction t(_odb->begin());
                _odb->erase<Relation>(odb::query<Relation>::user_id == uid1 && odb::query<Relation>::peer_id == uid2);
                _odb->erase<Relation>(odb::query<Relation>::user_id == uid2 && odb::query<Relation>::peer_id == uid1);
                t.commit();
            }
            catch (const std::exception &e)
            {
                LOG_ROOT_ERROR << "删除关系失败: " << e.what();
                return false;
            }
            return true;
        }
        //判断是否存在关系
        bool exist(const std::string& uid1,const std::string& uid2)
        {
            odb::result<Relation> r;
            try
            {
                odb::transaction t(_odb->begin());
                r=_odb->query_one<Relation>(odb::query<Relation>::user_id == uid1 && odb::query<Relation>::peer_id == uid2);
                t.commit();
            }
            catch (const std::exception &e)
            {
                LOG_ROOT_ERROR << "查询关系失败: " << e.what();
                return false;
            }
            return !r.empty();
        }
        //获取好友列表
        std::vector<std::string> get_friends(const std::string& uid)
        {
            std::vector<std::string> ret;
            try
            {
                odb::transaction t(_odb->begin());
                std::vector<odb::result<Relation>> r=_odb->query<Relation>(odb::query<Relation>::user_id == uid);
                for(auto& i:r)
                {
                    ret.emplace_back(i.peer_id());
                }
                t.commit();
            }
            catch (const std::exception &e)
            {
                LOG_ROOT_ERROR << "获取好友列表失败: " << e.what();
                return ret;
            }
            return ret;
        }
    private:
        std::shared_ptr<odb::mysql::database> _odb;
    };
}