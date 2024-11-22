#pragma once
#include <mysql/mysql.h>
#include <odb/mysql/mysql.hxx>
#include <odb/mysql/query.hxx>
#include <odb/mysql/database.hxx>
#include <odb/database.hxx>
#include "log.hpp"

namespace MindbniM
{
    class ODBFactory
    {
    public:
        /**
         * 创建并返回一个共享的MySQL数据库连接实例。
         * @param user 用户名
         * @param pswd 密码
         * @param host 数据库服务器地址
         * @param db 数据库名
         * @param cset 字符集
         * @param port 数据库端口
         * @param conn_pool_count 连接池大小
         * @return std::shared_ptr<odb::mysql::database> 返回数据库实例
         */
        static std::shared_ptr<odb::mysql::database> create(
            const std::string &user,
            const std::string &pswd,
            const std::string &host,
            const std::string &db,
            const std::string &cset, // 字符集
            int port,
            int conn_pool_count) // 数据库连接池数量
        {
            try
            {
                // 创建连接池工厂
                std::unique_ptr<odb::mysql::connection_pool_factory> cpf(
                    new odb::mysql::connection_pool_factory(conn_pool_count, 0) // 第二个参数0表示没有最大连接限制
                );

                // 创建数据库连接对象
                std::shared_ptr<odb::mysql::database> res = std::make_shared<odb::mysql::database>(
                    user, pswd, db, host, port, "", cset, 0, std::move(cpf));

                return res;
            }
            catch (const std::exception &e)
            {
                LOG_ROOT_ERROR << "数据库连接创建失败: " << e.what();
                return nullptr;
            }
        }
    };

}