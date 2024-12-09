#pragma once
#include "log.hpp"
#include "etcd.hpp"
#include "channel.hpp"
#include "user.pb.h"
#include "base.pb.h"
#include <brpc/server.h>
#include "es_user.hpp"
#include "mysql_user.hpp"
#include "redis_data.hpp"
#include "dms.hpp"
#include "uuid.hpp"
#include "file_service_client.hpp"
namespace MindbniM
{
    class UserServiceImpl : public UserService
    {
    public:
        UserServiceImpl(std::shared_ptr<odb::mysql::database> mysql, std::shared_ptr<sw::redis::Redis> redis,
                        std::shared_ptr<elasticlient::Client> es, ServiceManager::ptr service_manager, DMSClient::ptr dms)
        {
            _usertable = std::make_shared<UserTable>(mysql);
            _esuser = std::make_shared<ESUser>(es);
            _session = std::make_shared<Session>(redis);
            _status = std::make_shared<Status>(redis);
            _codes = std::make_shared<Codes>(redis);
            _service_manager = service_manager;
            _dms = dms;
        }

    private:
        bool nickname_check(const std::string &nickname)
        {
            if (nickname.size() < 1 || nickname.size() > 100)
            {
                return false;
            }
            return true;
        }
        bool password_check(const std::string &password)
        {
            if (password.size() < 6 || password.size() > 15)
                return false;
            return true;
        }
        bool phone_check(const std::string &phone)
        {
            if (phone.size() != 11 || phone[0] != '1' || phone[1] < '3' || phone[1] > '9')
                return false;
            for (auto &c : phone)
            {
                if (!isdigit(c))
                    return false;
            }
            return true;
        }
        int code_create()
        {
            std::random_device rd;
            std::mt19937 key(rd());
            std::uniform_int_distribution<int> dis(1000, 9999);
            return dis(key);
        }

    public:
        // 用户名注册
        void UserRegister(google::protobuf::RpcController *controller, const UserRegisterReq *req, UserRegisterRsp *rsp, google::protobuf::Closure *done)
        {
            LOG_ROOT_DEBUG << "用户注册请求";
            brpc::ClosureGuard guard(done);
            auto errctl = [req, rsp](const std::string &err) -> void
            {
                rsp->set_request_id(req->request_id());
                rsp->set_success(false);
                rsp->set_errmsg(err);
            };
            // 1. 从请求中取出昵称和密码
            std::string nickname = req->nickname();
            std::string password = req->password();
            // 2. 检查昵称是否合法（只能包含字母，数字，连字符-，下划线_，长度限制 3~15 之间）
            bool ret = nickname_check(nickname);
            if (!ret)
                return errctl("昵称太长或太短或包含非法字符");
            // 3. 检查密码是否合法（长度限制 6~15 之间）
            ret = password_check(password);
            if (!ret)
                return errctl("密码太长或太短");
            // 4. 根据昵称在数据库进行判断是否昵称已存在
            User::ptr p = _usertable->select_by_nickname(nickname);
            if (p)
                return errctl("昵称已存在");
            // 5. 向数据库新增数据
            std::string uid = UUID::Get();
            User newuser(uid, nickname, password);
            ret = _usertable->insert(newuser);
            if (!ret)
            {
                LOG_ROOT_ERROR << "mysql插入新用户 " << nickname << " 失败";
                return errctl("用户注册失败,未知原因");
            }
            // 6. 向 ES 服务器中新增用户信息
            ret = _esuser->Insert(newuser);
            if (!ret)
            {
                LOG_ROOT_ERROR << "es插入新用户 " << nickname << " 失败";
            }
            // 7. 组织响应，进行成功与否的响应即可
            rsp->set_request_id(req->request_id());
            rsp->set_success(true);
        }
        // 用户名登录
        void UserLogin(google::protobuf::RpcController *controller, const UserLoginReq *req, UserLoginRsp *rsp, google::protobuf::Closure *done)
        {
            LOG_ROOT_DEBUG << "用户名登录请求";
            brpc::ClosureGuard guard(done);
            auto errctl = [req, rsp](const std::string &err) -> void
            {
                rsp->set_request_id(req->request_id());
                rsp->set_success(false);
                rsp->set_errmsg(err);
            };
            // 1. 从请求中取出昵称和密码
            std::string nickname = req->nickname();
            std::string password = req->password();
            // 2. 通过昵称获取用户信息，进行密码是否一致的判断
            User::ptr user = _usertable->select_by_nickname(nickname);
            if (!user || *(user->_password) != password)
                return errctl("用户名或密码错误");
            // 3. 根据 redis 中的登录标记信息是否存在判断用户是否已经登录。如果存在就删除会话id,俗称挤号
            if (_status->exist(user->_user_id))
            {
                // session中 ssid->uid
                // status中 uid->status可互相拿到
                std::string ssid = _status->get(user->_user_id);
                _session->remove(ssid);
                _status->remove(user->_user_id);
            }
            // 4. 构造会话 ID，生成会话键值对，向 redis 中添加会话信息以及登录标记信息
            std::string ssid = UUID::Get();
            _session->append(ssid, user->_user_id);
            _status->append(user->_user_id, ssid);
            // 5. 组织响应，返回生成的会话
            rsp->set_request_id(req->request_id());
            rsp->set_success(true);
            rsp->set_login_session_id(ssid);
        }
        // 获取手机号验证码
        void GetPhoneVerifyCode(google::protobuf::RpcController *controller, const PhoneVerifyCodeReq *req, PhoneVerifyCodeRsp *rsp, google::protobuf::Closure *done)
        {
            LOG_ROOT_DEBUG << "获取手机号验证码请求";
            brpc::ClosureGuard guard(done);
            auto errctl = [req, rsp](const std::string &err) -> void
            {
                rsp->set_request_id(req->request_id());
                rsp->set_success(false);
                rsp->set_errmsg(err);
            };
            // 1. 从请求中取出手机号码
            std::string phone = req->phone_number();
            // 2. 验证手机号码格式是否正确（必须以 1 开始，第二位 3~9 之间，后边 9 个数字字符）
            bool ret = phone_check(phone);
            if (!ret)
                return errctl("非法手机号");
            // 3. 生成 4 位随机验证码
            std::string code = std::to_string(code_create());
            // 4. 基于短信平台 SDK 发送验证码
            _dms->Send(phone, code);
            // 5. 构造验证码 ID，添加到 redis 验证码映射键值索引中
            std::string code_uid = UUID::Get();
            _codes->append(code_uid, code);
            // 6. 组织响应，返回生成的验证码 ID
            rsp->set_request_id(req->request_id());
            rsp->set_success(true);
            rsp->set_verify_code_id(code_uid);
        }
        // 手机号注册
        void PhoneRegister(google::protobuf::RpcController *controller, const PhoneRegisterReq *req, PhoneRegisterRsp *rsp, google::protobuf::Closure *done)
        {
            LOG_ROOT_DEBUG << "手机号注册请求";
            brpc::ClosureGuard guard(done);
            auto errctl = [req, rsp](const std::string &err) -> void
            {
                rsp->set_request_id(req->request_id());
                rsp->set_success(false);
                rsp->set_errmsg(err);
            };
            // 1. 从请求中取出手机号码和验证码
            std::string phone = req->phone_number();
            std::string code = req->verify_code();
            // 2. 检查注册手机号码是否合法
            bool ret = phone_check(phone);
            if (!ret)
                return errctl("非法手机号");
            // 3. 从 redis 数据库中进行验证码 ID-验证码一致性匹配
            if (_codes->get(req->verify_code_id()) != code)
                return errctl("验证码错误");
            _codes->remove(req->verify_code_id());
            // 4. 通过数据库查询判断手机号是否已经注册过
            User::ptr p = _usertable->select_by_phone(phone);
            if (p)
                return errctl("手机号已注册");
            // 5. 向数据库新增用户信息
            std::string uid = UUID::Get();
            User newuser(uid, phone);
            _usertable->insert(newuser);
            if (!ret)
            {
                LOG_ROOT_ERROR << "mysql插入新用户 " << phone << " 失败";
                return errctl("用户注册失败,未知原因");
            }
            // 6. 向 ES 服务器中新增用户信息
            ret = _esuser->Insert(newuser);
            if (!ret)
            {
                LOG_ROOT_ERROR << "es插入新用户 " << phone << " 失败";
            }
            // 7. 组织响应，返回注册成功与否
            rsp->set_request_id(req->request_id());
            rsp->set_success(true);
        }
        // 手机号登录(可扩展用户不存在就注册)
        void PhoneLogin(google::protobuf::RpcController *controller, const PhoneLoginReq *req, PhoneLoginRsp *rsp, google::protobuf::Closure *done)
        {
            LOG_ROOT_DEBUG << "手机号登录请求";
            brpc::ClosureGuard guard(done);
            auto errctl = [req, rsp](const std::string &err) -> void
            {
                rsp->set_request_id(req->request_id());
                rsp->set_success(false);
                rsp->set_errmsg(err);
            };
            // 1. 从请求中取出手机号码和验证码 ID，以及验证码。
            std::string phone = req->phone_number();
            std::string code_id = req->verify_code_id();
            std::string code = req->verify_code();
            // 2. 检查注册手机号码是否合法
            bool ret = phone_check(phone);
            if (!ret)
                return errctl("非法手机号");
            // 3. 从 redis 数据库中进行验证码 ID-验证码一致性匹配
            if (_codes->get(code_id) != code)
                return errctl("验证码错误");
            // 4. 根据手机号从数据数据进行用户信息查询，判断用用户是否存在
            User::ptr user = _usertable->select_by_phone(phone);
            if (!user)
                return errctl("用户不存在");
            // 5. 根据 redis 中的登录标记信息是否存在判断用户是否已经登录。有就删除旧会话id
            if (_status->exist(user->_user_id))
            {
                std::string ssid = _status->get(user->_user_id);
                _session->remove(ssid);
                _status->remove(user->_user_id);
            }
            // 6. 构造会话 ID，生成会话键值对，向 redis 中添加会话信息以及登录标记信息
            std::string ssid = UUID::Get();
            _session->append(ssid, user->_user_id);
            _status->append(user->_user_id, ssid);
            // 7. 组织响应，返回生成的会话id
            rsp->set_request_id(req->request_id());
            rsp->set_success(true);
            rsp->set_login_session_id(ssid);
        }
        // 获取当前登录用户信息
        void GetUserInfo(google::protobuf::RpcController *controller, const GetUserInfoReq *req, GetUserInfoRsp *rsp, google::protobuf::Closure *done)
        {
            LOG_ROOT_DEBUG << "获取用户信息请求";
            brpc::ClosureGuard guard(done);
            auto errctl = [req, rsp](const std::string &err) -> void
            {
                rsp->set_request_id(req->request_id());
                rsp->set_success(false);
                rsp->set_errmsg(err);
            };
            // 1. 从请求中取出用户 ID
            std::string uid = req->user_id();
            // 2. 通过用户 ID，从数据库中查询用户信息
            User::ptr user = _usertable->select_by_id(uid);
            if (!user)
            {
                LOG_ROOT_ERROR << "已登录用户 " << uid << " 不存在";
                return errctl("用户不存在");
            }
            UserInfo *ret = rsp->mutable_user_info();
            ret->set_user_id(user->_user_id);
            if (user->_nickname)
                ret->set_nickname(*user->_nickname);
            if (user->_phone)
                ret->set_phone(*user->_phone);
            if (user->_description)
                ret->set_description(*user->_description);
            // 3. 根据用户信息中的头像 ID，从文件服务器获取头像文件数据，组织完整用户信息
            if (user->_avatar_id)
            {
                std::shared_ptr<brpc::Channel> channel = _service_manager->choose(FileService);
                if (channel == nullptr)
                {
                    LOG_ROOT_ERROR << "文件存储子服务未找到";
                    return errctl("文件服务器未找到");
                }
                std::string file_content;
                FileServiceClient client(channel);
                bool b = client.GetSingleFile(*user->_avatar_id, file_content);
                if (!b)
                    return errctl("获取用户头像失败");
                ret->set_avatar(file_content);
            }
            // 4. 组织响应，返回用户信息
            rsp->set_request_id(req->request_id());
            rsp->set_success(true);
        }
        // 获取多个用户信息
        void GetMultiUserInfo(google::protobuf::RpcController *controller, const GetMultiUserInfoReq *req, GetMultiUserInfoRsp *rsp, google::protobuf::Closure *done)
        {
            LOG_ROOT_DEBUG << "获取多个用户信息请求";
            brpc::ClosureGuard guard(done);
            auto errctl = [req, rsp](const std::string &err) -> void
            {
                rsp->set_request_id(req->request_id());
                rsp->set_success(false);
                rsp->set_errmsg(err);
            };
            // 获取用户id列表
            std::vector<std::string> id_lists(req->users_id().begin(), req->users_id().end());
            // 获取多个用户信息
            std::vector<User> users = _usertable->select_multi_users(id_lists);
            if (users.size() != req->users_id_size())
            {
                LOG_ROOT_ERROR << "从数据库查找的用户信息数量不一致" << req->request_id() << "select user size:" << req->users_id_size() << "mysql user size:" << users.size();
                return errctl("从数据库查找的用户信息数量不一致!");
            }
            std::shared_ptr<brpc::Channel> channel = _service_manager->choose(FileService);
            FileServiceClient client(channel);
            if (channel == nullptr)
            {
                LOG_ROOT_ERROR << "文件存储子服务未找到";
                return errctl("文件服务器未找到");
            }
            auto *user_map = rsp->mutable_users_info();
            for (const auto &user : users)
            {
                auto &ret = (*user_map)[user._user_id];
                ret.set_user_id(user._user_id);
                if (user._nickname)
                    ret.set_nickname(*user._nickname);
                if (user._phone)
                    ret.set_phone(*user._phone);
                if (user._description)
                    ret.set_description(*user._description);
                if (user._avatar_id)
                {
                    std::string file_content;
                    if (!client.GetSingleFile(*user._avatar_id, file_content))
                    {
                        return errctl("获取用户头像失败");
                    }
                    ret.set_avatar(std::move(file_content));
                }
            }
            rsp->set_request_id(req->request_id());
            rsp->set_success(true);
        }
        // 设置用户头像
        void SetUserAvatar(google::protobuf::RpcController *controller, const SetUserAvatarReq *req, SetUserAvatarRsp *rsp, google::protobuf::Closure *done)
        {
            LOG_ROOT_DEBUG << "设置用户头像请求";
            brpc::ClosureGuard guard(done);
            auto errctl = [req, rsp](const std::string &err) -> void
            {
                rsp->set_request_id(req->request_id());
                rsp->set_success(false);
                rsp->set_errmsg(err);
            };
            //1. 从请求中取出用户 ID 与头像数据
            std::string user_id=req->user_id();
            std::string file_content=req->avatar();
            //2. 从数据库通过用户 ID 进行用户信息查询，判断用户是否存在
            User::ptr p=_usertable->select_by_id(user_id);
            if(!p) return errctl("用户不存在");
            //3. 上传头像文件到文件子服务，
            std::shared_ptr<brpc::Channel> channel=_service_manager->choose(FileService);
            if (channel == nullptr)
            {
                LOG_ROOT_ERROR << "文件存储子服务未找到";
                return errctl("文件服务器未找到");
            }
            FileServiceClient client(channel);
            std::string file_id;
            bool ret=client.PutSingleFile(p->_user_id,file_content,file_id);
            if(!ret) return errctl("上传头像失败");
            //4. 将返回的头像文件 ID 更新到数据库中
            p->_avatar_id=file_id;
            LOG_ROOT_DEBUG<<"获得头像文件id:"<<file_id;
            ret=_usertable->update(p);
            if(!ret)
            {
                LOG_ROOT_ERROR<<"更新用户头像失败";
                return errctl("更新用户头像失败");
            }
            //5. 更新 ES 服务器中用户信息
            ret=_esuser->Insert(*p);
            if(!ret)
            {
                LOG_ROOT_ERROR<<"更新 ES 服务器用户头像失败";
                return errctl("更新 ES 服务器用户头像失败");
            }
            //6. 组织响应，返回更新成功与否
            rsp->set_request_id(req->request_id());
            rsp->set_success(true);
        }
        // 设置用户昵称
        void SetUserNickname(google::protobuf::RpcController *controller, const SetUserNicknameReq *req, SetUserNicknameRsp *rsp, google::protobuf::Closure *done)
        {
            LOG_ROOT_DEBUG << "设置用户昵称请求";
            brpc::ClosureGuard guard(done);
            auto errctl = [req, rsp](const std::string &err) -> void
            {
                rsp->set_request_id(req->request_id());
                rsp->set_success(false);
                rsp->set_errmsg(err);
            };
            //1. 从请求中取出用户 ID 与昵称
            std::string user_id=req->user_id();
            std::string nickname=req->nickname();
            //2. 检查昵称是否合法
            bool ret=nickname_check(nickname);
            if(!ret) return errctl("昵称不合法");
            //3. 从数据库通过用户 ID 进行用户信息查询，判断用户是否存在
            User::ptr p=_usertable->select_by_id(user_id);
            if(!p) return errctl("用户不存在");
            //4. 更新用户昵称
            p->_nickname=nickname;
            ret=_usertable->update(p);
            if(!ret)
            {
                LOG_ROOT_ERROR<<"更新用户昵称失败";
                return errctl("更新用户昵称失败");
            }
            //5. 更新 ES 服务器中用户信息
            ret=_esuser->Insert(*p);
            if(!ret)
            {
                LOG_ROOT_ERROR<<"更新 ES 服务器用户昵称失败";
                return errctl("更新 ES 服务器用户昵称失败");
            }
            //6. 组织响应，返回更新成功与否
            rsp->set_request_id(req->request_id());
            rsp->set_success(true);
        }
        // 设置用户描述
        void SetUserDescription(google::protobuf::RpcController *controller, const SetUserDescriptionReq *req, SetUserDescriptionRsp *rsp, google::protobuf::Closure *done)
        {
            LOG_ROOT_DEBUG << "设置用户描述请求";
            brpc::ClosureGuard guard(done);
            auto errctl = [req, rsp](const std::string &err) -> void
            {
                rsp->set_request_id(req->request_id());
                rsp->set_success(false);
                rsp->set_errmsg(err);
            };
            //1. 从请求中取出用户 ID 与描述
            std::string user_id=req->user_id();
            std::string description=req->description();
            //2. 从数据库通过用户 ID 进行用户信息查询，判断用户是否存在
            User::ptr p=_usertable->select_by_id(user_id);
            if(!p) return errctl("用户不存在");
            //3. 更新用户描述
            p->_description=description;
            bool ret=_usertable->update(p);
            if(!ret)
            {
                LOG_ROOT_ERROR<<"更新用户描述失败";
                return errctl("更新用户描述失败");
            }
            //4. 更新 ES 服务器中用户信息
            ret=_esuser->Insert(*p);
            if(!ret)
            {
                LOG_ROOT_ERROR<<"更新 ES 服务器用户描述失败";
                return errctl("更新 ES 服务器用户描述失败");
            }
            //5. 组织响应，返回更新成功与否
            rsp->set_request_id(req->request_id());
            rsp->set_success(true);
        }
        // 设置用户手机号
        void SetUserPhoneNumber(google::protobuf::RpcController *controller, const SetUserPhoneNumberReq *req, SetUserPhoneNumberRsp *rsp, google::protobuf::Closure *done)
        {
            brpc::ClosureGuard guard(done);
            auto errctl = [req, rsp](const std::string &err) -> void
            {
                rsp->set_request_id(req->request_id());
                rsp->set_success(false);
                rsp->set_errmsg(err);
            };
            //1. 从请求中取出手机号码和验证码 ID，以及验证码。
            std::string phone=req->phone_number();
            std::string code_id=req->phone_verify_code_id();
            std::string code=req->phone_verify_code();
            //2. 检查注册手机号码是否合法
            bool ret=phone_check(phone);
            if(!ret) return errctl("非法手机号");
            //3. 从 redis 数据库中进行验证码 ID-验证码一致性匹配
            if(_codes->get(code_id)!=code) return errctl("验证码错误");
            //4. 根据用户id从数据数据进行用户信息查询，判断用用户是否存在
            User::ptr user=_usertable->select_by_id(req->user_id());
            if(!user) return errctl("用户不存在");
            //5. 将新的手机号更新到数据库中
            user->_phone=phone;
            ret=_usertable->update(user);
            if(!ret)
            {
                LOG_ROOT_ERROR<<"更新用户手机号失败";
                return errctl("更新用户手机号失败");
            }
            //6. 更新 ES 服务器中用户信息
            ret=_esuser->Insert(*user);
            if(!ret)
            {
                LOG_ROOT_ERROR<<"更新 ES 服务器用户手机号失败";
                return errctl("更新 ES 服务器用户手机号失败");
            }
            //7. 组织响应，返回更新成功与否
            rsp->set_request_id(req->request_id());
            rsp->set_success(true);
        }

    private:
        UserTable::ptr _usertable;
        ESUser::ptr _esuser;

        Session::ptr _session;
        Status::ptr _status;
        Codes::ptr _codes;

        ServiceManager::ptr _service_manager;
        const std::string FileService = "/service/file_service";

        DMSClient::ptr _dms;
    };

    class UserServer
    {
    public:
        using ptr = std::shared_ptr<UserServer>;
        UserServer(Registry::ptr reg, std::shared_ptr<brpc::Server> rpc) : _reg_client(reg), _rpc_server(rpc)
        {
        }
        void start()
        {
            _rpc_server->RunUntilAskedToQuit();
        }

    private:
        Registry::ptr _reg_client;
        std::shared_ptr<brpc::Server> _rpc_server;
        // Discovery::ptr _discover;
        // std::shared_ptr<odb::mysql::database> _mysql;
        // std::shared_ptr<sw::redis::Redis> _redis;
        // std::shared_ptr<elasticlient::Client> _es;
    };
    class UserServerBuild
    {
    public:
        void make_reg_object(const std::string &reg_host, const std::string &service_name, const std::string &service_host)
        {
            _reg_client = std::make_shared<Registry>(reg_host);
            _reg_client->registry(service_name, service_host);
        }
        void make_dis_object(const std::string &dis_host, const std::string& dis_dir)
        {
            _service_manager = std::make_shared<ServiceManager>();
            _service_manager->add_concern(dis_dir);
            auto put_cb = std::bind(&ServiceManager::onServiceOnline, _service_manager.get(), std::placeholders::_1, std::placeholders::_2);
            auto del_cb = std::bind(&ServiceManager::onServiceOffline, _service_manager.get(), std::placeholders::_1, std::placeholders::_2);
            _discover= std::make_shared<Discovery>(dis_host, put_cb, del_cb);
            _discover->discover(dis_dir);
        }
        void make_mysql_object(const std::string &host, int port, const std::string &user, const std::string &passwd, const std::string &db, const std::string &set, int poll_count)
        {
            _mysql = ODBFactory::create(user, passwd, host, db, set, port, poll_count);
        }
        void make_redis_object(const std::string &host, int port, int db)
        {
            _redis = RedisFactory::create(host, port, db);
        }
        void make_es_object(const std::vector<std::string> &hosts)
        {
            _es = std::make_shared<elasticlient::Client>(hosts);
        }
        // rpc服务器应该最后调用
        void make_rpc_server(uint16_t port, int timeout, int thread_num)
        {
            _rpc_server = std::make_shared<brpc::Server>();
            _dms = std::make_shared<DMSClient>();
            if (_mysql == nullptr)
            {
                LOG_ROOT_ERROR << "未初始化mysql数据库";
                abort();
            }
            if (_redis == nullptr)
            {
                LOG_ROOT_ERROR << "未初始化redis数据库";
                abort();
            }
            if (_es == nullptr)
            {
                LOG_ROOT_ERROR << "未初始化es数据库";
                abort();
            }
            if (_service_manager == nullptr)
            {
                LOG_ROOT_ERROR << "未初始化服务发现模块";
                abort();
            }
            if (_dms == nullptr)
            {
                LOG_ROOT_ERROR << "未初始化短信模块";
                abort();
            }
            UserServiceImpl *user_service = new UserServiceImpl(_mysql, _redis, _es, _service_manager, _dms);
            int ret = _rpc_server->AddService(user_service, brpc::ServiceOwnership::SERVER_OWNS_SERVICE);
            if (ret < 0)
            {
                LOG_ROOT_ERROR << "添加rpc服务失败";
                return;
            }
            brpc::ServerOptions op;
            op.idle_timeout_sec = timeout;
            op.num_threads = thread_num;
            ret = _rpc_server->Start(port, &op);
            if (ret < 0)
            {
                LOG_ROOT_ERROR << "rpc服务启动失败";
                return;
            }
        }
        MindbniM::UserServer::ptr newUserServer()
        {
            return std::make_shared<UserServer>(_reg_client, _rpc_server);
        }

    private:
        Registry::ptr _reg_client;
        Discovery::ptr _discover;
        std::shared_ptr<brpc::Server> _rpc_server;
        std::shared_ptr<odb::mysql::database> _mysql;
        std::shared_ptr<sw::redis::Redis> _redis;
        std::shared_ptr<elasticlient::Client> _es;
        ServiceManager::ptr _service_manager;

        DMSClient::ptr _dms;
    };
};
