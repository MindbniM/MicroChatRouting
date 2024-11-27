#pragma once
#include <odb/query.hxx>
#include "odb.hpp"
#include "frient_apply-odb.hxx"
#include<odb/core.hxx>
#include <odb/result.hxx>
namespace MindbniM
{
    class FriendApplyTable
    {
    public:
        using ptr=std::shared_ptr<FriendApplyTable>;
        FriendApplyTable(const std::shared_ptr<odb::mysql::database> &odb) : _odb(odb)
        {}
        //新增好友申请
        bool insert(FriendApply& fa)
        {
            try
            {
                odb::transaction t(_odb->begin());
                _odb->persist(fa);
                t.commit();
            }
            catch (const std::exception &e)
            {
                LOG_ROOT_ERROR << "新增好友申请失败: " << e.what();
                return false;
            }
            return true;
        }
        //删除好友申请
        bool remove(const std::string& uid,const std::string& pid)
        {
            try
            {
                odb::transaction t(_odb->begin());
                _odb->erase<FriendApply>(odb::query<FriendApply>::user_id == uid && odb::query<FriendApply>::peer_id == pid);
                t.commit();
            }
            catch (const std::exception &e)
            {
                LOG_ROOT_ERROR << "删除好友申请失败: " << e.what();
                return false;
            }
            return true;
        }
        //获取指定用户的好友申请者的用户ID
        std::vector<std::string> query(const std::string& uid)
        {
            std::vector<std::string> ret;
            try
            {
                odb::transaction t(_odb->begin());
                odb::result<FriendApply> r=_odb->query<FriendApply>(odb::query<FriendApply>::peer_id == uid);
                for(auto& i:r)
                {
                    ret.emplace_back(i.user_id());
                }
                t.commit();
            }
            catch (const std::exception &e)
            {
                LOG_ROOT_ERROR << "查询好友申请失败: " << e.what();
                return {};
            }
            return ret;
        }
    private:
        std::shared_ptr<odb::mysql::database> _odb;
    };
}
