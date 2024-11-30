#pragma once
#include "etcd.hpp"
#include "channel.hpp"
#include <brpc/server.h>
#include "mysql_relation.hpp"
#include"mysql_chat_session.hpp"
#include"mysql_friendapply.hpp"
#include"mysql_chat_session_member.hpp"
#include"es_user.hpp"
#include"user_service_client.hpp"
#include"message_service_client.hpp"
#include"friend.pb.h"

namespace MindbniM
{
    class FriendServiceImpl : public FriendService
    {
    public:
        FriendServiceImpl(std::shared_ptr<odb::mysql::database> mysql,std::shared_ptr<elasticlient::Client> es, ServiceManager::ptr sm) : _service_manager(sm)
        {
            _chat_session_table = std::make_shared<ChatSessionTable>(mysql);
            _friend_apply_table = std::make_shared<FriendApplyTable>(mysql);
            _relation_table = std::make_shared<RelationTable>(mysql);
            _chat_session_member_table=std::make_shared<ChatSessionMemberTable>(mysql);
            _es=std::make_shared<ESUser>(es);
            _service_manager=sm;
        }
        void GetFriendList(google::protobuf::RpcController *controller, const GetFriendListReq *req,GetFriendListRsp *rsp, google::protobuf::Closure *done)
        {
            brpc::ClosureGuard done_guard(done);
            auto errctl=[req,rsp](const std::string err)
            {
                rsp->set_request_id(req->request_id());
                rsp->set_success(false);
                rsp->set_errmsg(err);
            };
            std::string uid=req->user_id();
            //1.获取用户的好友ID列表
            std::vector<std::string> fids=_relation_table->get_friends(uid);
            //2.从用户子服务获取好友信息
            std::shared_ptr<brpc::Channel> channel=_service_manager->choose(_user_service);
            if(channel==nullptr)
            {
                LOG_ROOT_ERROR<<"获取用户子服务失败";
                return errctl("获取用户子服务失败");
            }
            UserServiceClient client(channel);
            std::vector<UserInfo> friends;
            bool ret=client.GetMultiUserInfo(fids,friends);
            if(!ret)
            {
                LOG_ROOT_ERROR<<"获取好友信息失败";
                return errctl("获取好友信息失败");
            }
            //3.组织响应
            rsp->set_request_id(req->request_id());
            rsp->set_success(true);
            for (const auto & user: friends) 
            {
                auto user_info = rsp->add_friend_list();
                user_info->CopyFrom(user);
            }
        }
        void FriendRemove(google::protobuf::RpcController *controller, const FriendRemoveReq *req,FriendRemoveRsp *rsp, google::protobuf::Closure *done)
        {
            brpc::ClosureGuard done_guard(done);
            auto errctl=[req,rsp](const std::string err)
            {
                rsp->set_request_id(req->request_id());
                rsp->set_success(false);
                rsp->set_errmsg(err);
            };
            //1.删除好友关系
            std::string fid=req->peer_id();
            bool ret=_relation_table->remove(req->user_id(),fid);
            if(!ret)
            {
                LOG_ROOT_ERROR<<"删除好友关系失败";
                return errctl("删除好友关系失败");
            }
            //2.删除会话信息
            ret=_chat_session_table->remove(req->user_id(),fid);
            if(!ret)
            {
                LOG_ROOT_ERROR<<"删除会话信息失败";
                return errctl("删除会话信息失败");
            }
            //3.组织响应
            rsp->set_request_id(req->request_id());
            rsp->set_success(true);
        }
        //发送好友添加申请
        void FriendAdd(google::protobuf::RpcController *controller, const FriendAddReq *req,FriendAddRsp *rsp, google::protobuf::Closure *done)
        {
            brpc::ClosureGuard done_guard(done);
            auto errctl=[req,rsp](const std::string err)
            {
                rsp->set_request_id(req->request_id());
                rsp->set_success(false);
                rsp->set_errmsg(err);
            };
            std::string uid=req->user_id();
            std::string pid=req->respondent_id();
            //1.判断是否已经是好友
            if(_relation_table->exist(uid,pid))
            {
                LOG_ROOT_ERROR<<"已经是好友";
                return errctl("已经是好友了");
            }
            //2.判断是否已经发送过申请
            if(_friend_apply_table->exist(uid,pid))
            {
                LOG_ROOT_ERROR<<"已经发送过好友申请";
                return errctl("已经发送过好友申请");
            }
            //3. 向好友申请事件表中，新增申请信息
            FriendApply fa(uid,pid);
            bool ret=_friend_apply_table->insert(fa);
            if(!ret)
            {
                LOG_ROOT_ERROR<<"好友申请失败";
                return errctl("好友申请失败");
            }
            //4. 组织响应
            rsp->set_request_id(req->request_id());
            rsp->set_success(true);
        }
        void FriendAddProcess(google::protobuf::RpcController *controller, const FriendAddProcessReq *req,FriendAddProcessRsp *rsp, google::protobuf::Closure *done)
        {
            brpc::ClosureGuard done_guard(done);
            auto errctl=[req,rsp](const std::string err)
            {
                rsp->set_request_id(req->request_id());
                rsp->set_success(false);
                rsp->set_errmsg(err);
            };
            std::string uid=req->apply_user_id();
            std::string pid=req->user_id();
            bool agree=req->agree();
            //1. 根据两人 ID 在申请事件表中查询判断是否存在申请事件
            if(!_friend_apply_table->exist(uid,pid))
            {
                LOG_ROOT_ERROR<<"不存在好友申请事件";
                return errctl("不存在好友申请事件");
            }
            //2. 判断两人是否已经是好友（互相加好友的情况）
            if(_relation_table->exist(uid,pid))
            {
                LOG_ROOT_ERROR<<"已经是好友";
                return errctl("已经是好友");
            }
            //3. 不管拒绝还是同意，删除申请事件表中的事件信息（该事件处理完毕）
            bool ret=_friend_apply_table->remove(uid,pid);
            if(!ret)
            {
                LOG_ROOT_ERROR<<"删除好友申请事件失败";
                return errctl("删除好友申请事件失败");
            }
            //4. 若同意申请，则向用户关系表中添加好友关系数据，向会话表中新增会话信息，向会话成员表中新增成员信息
            std::string sid;
            if(agree)
            {
                sid=UUID::Get();
                ret=_relation_table->insert(uid,pid);
                if(!ret)
                {
                    LOG_ROOT_ERROR<<"添加好友关系失败";
                    return errctl("添加好友关系失败");
                }
                ChatSession cs(sid,"",ChatSessionType::SINGLE);
                ret=_chat_session_table->Insert(cs);
                if(!ret)
                {
                    LOG_ROOT_ERROR<<"添加会话信息失败";
                    return errctl("添加会话信息失败");
                }
                ChatSessionMember csm1(sid,uid);
                ChatSessionMember csm2(sid,pid);
                std::vector<ChatSessionMember> csms{csm1,csm2};
                ret=_chat_session_member_table->insert(csms);
                if(!ret)
                {
                    LOG_ROOT_ERROR<<"添加会话成员失败";
                    return errctl("添加会话成员失败");
                }
            }
            //5. 组织响应
            rsp->set_request_id(req->request_id());
            rsp->set_success(true);
            rsp->set_new_session_id(sid);
        }
        void FriendSearch(google::protobuf::RpcController *controller, const FriendSearchReq *req,FriendSearchRsp *rsp, google::protobuf::Closure *done)
        {
            brpc::ClosureGuard done_guard(done);
            auto errctl=[req,rsp](const std::string err)
            {
                rsp->set_request_id(req->request_id());
                rsp->set_success(false);
                rsp->set_errmsg(err);
            };
            std::string uid=req->user_id();
            std::string key=req->search_key();
            //1. 从好友关系表中取出该用户所有好友 ID
            std::vector<std::string> fids=_relation_table->get_friends(uid);
            //2. 根据关键字从 ES 服务器中进行用户搜索，搜索的时候需要将关键字作为用户 ID/手机号/昵称的搜索关键字进行搜索，且需要根据自己的 ID 和好友 ID 过滤掉自己和自己的好友。
            std::unordered_set<std::string> user_id_lists;
            fids.push_back(uid);// 把自己也过滤掉
            std::vector<User> search_res = _es->Search(key, fids);
            for (auto &it : search_res) 
            {
                user_id_lists.insert(it._user_id);
            }
            //3. 根据搜索到的用户ID，从用户子服务器进行批量用户信息获取
            std::shared_ptr<brpc::Channel> channel=_service_manager->choose(_user_service);
            if(channel==nullptr)
            {
                LOG_ROOT_ERROR<<"获取用户子服务失败";
                return errctl("获取用户子服务失败");
            }
            UserServiceClient client(channel);
            std::vector<UserInfo> friends;
            bool ret=client.GetMultiUserInfo(fids,friends);
            if(!ret)
            {
                LOG_ROOT_ERROR<<"获取好友信息失败";
                return errctl("获取好友信息失败");
            }
            //4. 组织响应，将搜索到的用户列表响应
            rsp->set_request_id(req->request_id());
            rsp->set_success(true);
            for (const auto & user: friends) 
            {
                auto user_info = rsp->add_user_info();
                user_info->CopyFrom(user);
            }
        }
        void GetPendingFriendEventList(google::protobuf::RpcController *controller, const GetPendingFriendEventListReq *req,GetPendingFriendEventListRsp *rsp, google::protobuf::Closure *done)
        {
            brpc::ClosureGuard done_guard(done);
            auto errctl=[req,rsp](const std::string err)
            {
                rsp->set_request_id(req->request_id());
                rsp->set_success(false);
                rsp->set_errmsg(err);
            };
            std::string uid=req->user_id();
            //1. 根据用户 ID，从申请事件表&用户表中找到该用户所有状态为 PENDING 的待处理事件关联申请人用户简息
            std::vector<std::string> fids=_friend_apply_table->get_friends(uid);
            //2. 根据申请人用户头像 ID，从文件存储子服务器获取所有用户头像信息，组织用户信息结构
            std::shared_ptr<brpc::Channel> channel=_service_manager->choose(_user_service);
            if(channel==nullptr)
            {
                LOG_ROOT_ERROR<<"获取用户子服务失败";
                return errctl("获取用户子服务失败");
            }
            UserServiceClient client(channel);
            std::vector<UserInfo> friends;
            bool ret=client.GetMultiUserInfo(fids,friends);
            if(!ret)
            {
                LOG_ROOT_ERROR<<"获取好友信息失败";
                return errctl("获取好友信息失败");
            }
            //3. 组织响应，将申请事件列表响应给网关
            rsp->set_request_id(req->request_id());
            rsp->set_success(true);
            for (const auto & user: friends) 
            {
                auto user_info = rsp->add_sender();
                user_info->CopyFrom(user);
            }
        }
        void GetChatSessionList(google::protobuf::RpcController *controller, const GetChatSessionListReq *req,GetChatSessionListRsp *rsp, google::protobuf::Closure *done)
        {
            brpc::ClosureGuard done_guard(done);
            auto errctl=[req,rsp](const std::string err)
            {
                rsp->set_request_id(req->request_id());
                rsp->set_success(false);
                rsp->set_errmsg(err);
            };
            //1. 从请求中取出用户 ID
            std::string uid=req->user_id();
            //2. 根据用户 ID，从会话表&会话成员表&用户表中取出好友的单聊会话列表（会话 ID，好友用户 ID，好友昵称，好友头像 ID），并组织会话信息结构对象
            std::vector<SingleChatSession> friends=_chat_session_table->GetSingChatSession(uid);
            std::vector<std::string> fids;
            std::vector<std::string> sids;
            for(auto& f:friends)
            {
                fids.push_back(f.friend_id);
                sids.push_back(f.chat_session_id);
            }
            std::shared_ptr<brpc::Channel> channel=_service_manager->choose(_user_service);
            if(channel==nullptr)
            {
                LOG_ROOT_ERROR<<"未找到用户子服务";
                return errctl("未找到用户子服务");
            }
            UserServiceClient Userclient(channel);
            std::vector<UserInfo> users;
            bool ret=Userclient.GetMultiUserInfo(fids,users);
            if(!ret)
            {
                LOG_ROOT_ERROR<<"获取用户信息失败";
                return errctl("获取用户信息失败");
            }
            channel=_service_manager->choose(_message_service);
            if(channel==nullptr)
            {
                LOG_ROOT_ERROR<<"未找到消息存储子服务";
                return errctl("未找到消息存储子服务");
            }
            MessageServiceClient Msgclient(channel);
            std::vector<MessageInfo> messages;
            for(int i=0;i<fids.size();i++)
            {
                auto info=rsp->add_chat_session_info_list();
                info->set_single_chat_friend_id(fids[i]);
                info->set_chat_session_id(sids[i]);
                info->set_chat_session_name(users[i].nickname());
                bool ret=Msgclient.GetRecentMsg(sids[i],1,messages);
                if(!ret)
                {
                    LOG_ROOT_ERROR<<"获取最近消息失败";
                    return errctl("获取最近消息失败");
                }
                info->mutable_prev_message()->CopyFrom(messages[0]);
                info->set_avatar(users[i].avatar());
            }
            //群聊会话列表
            std::vector<GroupChatSession> groups=_chat_session_table->GetGroupChatSession(uid);
            //3. 根据单聊会话 ID，从消息存储子服务获取会话的最后一条消息
            for(auto& g:groups)
            {
                auto info=rsp->add_chat_session_info_list();
                info->set_chat_session_id(g.chat_session_id);
                info->set_chat_session_name(g.chat_session_name);
                bool ret=Msgclient.GetRecentMsg(g.chat_session_id,1,messages);
                if(!ret)
                {
                    LOG_ROOT_ERROR<<"获取最近消息失败";
                    return errctl("获取最近消息失败");
                }
                info->mutable_prev_message()->CopyFrom(messages[0]);
            }
            //4. 组织响应
            rsp->set_request_id(req->request_id());
            rsp->set_success(true);
        }
        void ChatSessionCreate(google::protobuf::RpcController *controller, const ChatSessionCreateReq *req,ChatSessionCreateRsp *rsp, google::protobuf::Closure *done)
        {
            brpc::ClosureGuard done_guard(done);
            auto errctl=[req,rsp](const std::string err)
            {
                rsp->set_request_id(req->request_id());
                rsp->set_success(false);
                rsp->set_errmsg(err);
            };
            std::string sid=UUID::Get();
            //1. 生成会话 ID，并向会话表中新增会话信息数据，会话为群聊会话（单聊会话是同意好友申请的时候创建的）
            ChatSession cs(sid,req->chat_session_name(),ChatSessionType::GROUP);
            bool ret=_chat_session_table->Insert(cs);
            //2. 向会话成员表中新增所有的成员信息
            std::vector<ChatSessionMember> users;
            for(auto& uid:req->member_id_list())
            {
                ChatSessionMember csm(sid,uid);
                users.emplace_back(csm);
            }
            ret=_chat_session_member_table->insert(users);
            if(!ret)
            {
                LOG_ROOT_ERROR<<"添加会话成员失败";
                return errctl("添加会话成员失败");
            }
            //3. 组织响应，将组织好的会话信息响应给网关。
            rsp->set_request_id(req->request_id());
            rsp->set_success(true);
            auto info=rsp->mutable_chat_session_info();
            info->set_chat_session_id(sid);
            info->set_chat_session_name(req->chat_session_name());
        }
        void GetChatSessionMember(google::protobuf::RpcController *controller, const GetChatSessionMemberReq *req,GetChatSessionMemberRsp *rsp, google::protobuf::Closure *done)
        {
            brpc::ClosureGuard done_guard(done);
            auto errctl=[req,rsp](const std::string err)
            {
                rsp->set_request_id(req->request_id());
                rsp->set_success(false);
                rsp->set_errmsg(err);
            };
            std::string sid=req->chat_session_id();
            //1. 根据会话 ID，从会话成员表&用户表中取出所有的成员用户信息
            vector<ChatSessionMember> u=_chat_session_member_table->get_members(sid);
            std::vector<std::string> uids;
            for(auto& i:u)
            {
                uids.push_back(i._user_id);
            }
            std::shared_ptr<brpc::Channel> channel=_service_manager->choose(_user_service);
            if(channel==nullptr)
            {
                LOG_ROOT_ERROR<<"未找到用户子服务";
                return errctl("未找到用户子服务");
            }
            UserServiceClient client(channel);
            std::vector<UserInfo> users;
            bool ret=client.GetMultiUserInfo(uids,users);
            if(!ret)
            {
                LOG_ROOT_ERROR<<"获取用户信息失败";
                return errctl("获取用户信息失败");
            }
            //2. 组织响应，将会话的成员用户信息列表响应给网关
            rsp->set_request_id(req->request_id());
            rsp->set_success(true);
            for(auto& user:users)
            {
                auto info=rsp->add_member_info_list();
                info->CopyFrom(user);
            }
        }

    private:
        ServiceManager::ptr _service_manager;

        ChatSessionMemberTable::ptr _chat_session_member_table;
        ChatSessionTable::ptr _chat_session_table;
        FriendApplyTable::ptr _friend_apply_table;
        RelationTable::ptr _relation_table;
        const std::string _user_service="/service/user_service";
        const std::string _message_service="/service/message_service";

        ESUser::ptr _es;
    };

    class FriendServer
    {
    public:
        using ptr = std::shared_ptr<FriendServer>;
        FriendServer(Registry::ptr reg, std::shared_ptr<brpc::Server> rpc) : _reg_client(reg), _rpc_server(rpc)
        {
        }
        void start()
        {
            _rpc_server->RunUntilAskedToQuit();
        }

    private:
        Registry::ptr _reg_client;
        std::shared_ptr<brpc::Server> _rpc_server;
    };
    class FriendServerBuild
    {
    public:
        void make_reg_object(const std::string &reg_host, const std::string &service_name, const std::string &service_host)
        {
            _reg_client = std::make_shared<Registry>(reg_host);
            _reg_client->registry(service_name, service_host);
        }
        void make_dis_object(const std::string &dis_host, const std::string base_dir,std::vector<std::string> dis_dir)
        {
            _service_manager = std::make_shared<ServiceManager>();
            for(auto& dir:dis_dir)
            {
                _service_manager->add_concern(dir);
            }
            auto put_cb = std::bind(&ServiceManager::onServiceOnline, _service_manager.get(), std::placeholders::_1, std::placeholders::_2);
            auto del_cb = std::bind(&ServiceManager::onServiceOffline, _service_manager.get(), std::placeholders::_1, std::placeholders::_2);
            _discover= std::make_shared<Discovery>(dis_host, put_cb, del_cb);
            _discover->discover(base_dir);
        }
        void make_mysql_object(const std::string &host, int port, const std::string &user, const std::string &passwd, const std::string &db, const std::string &set, int poll_count)
        {
            _mysql = ODBFactory::create(user, passwd, host, db, set, port, poll_count);
        }
        void make_es_object(const std::vector<std::string> &hosts)
        {
            _es = std::make_shared<elasticlient::Client>(hosts);
        }
        // rpc服务器应该最后调用
        void make_rpc_server(uint16_t port, int timeout, int thread_num)
        {
            _rpc_server = std::make_shared<brpc::Server>();
            if (_mysql == nullptr)
            {
                LOG_ROOT_ERROR << "未初始化mysql数据库";
                abort();
            }
            if (_es == nullptr)
            {
                LOG_ROOT_ERROR << "未初始化es数据库";
                abort();
            }
            if(_reg_client==nullptr)
            {
                LOG_ROOT_ERROR<<"未初始化注册中心";
                abort();
            }
            if(_discover==nullptr)
            {
                LOG_ROOT_ERROR<<"未初始化服务发现模块";
                abort();
            }
            if (_service_manager == nullptr)
            {
                LOG_ROOT_ERROR << "未初始化服务发现模块";
                abort();
            }
            FriendServiceImpl *friend_service = new FriendServiceImpl(_mysql,_es, _service_manager);
            int ret = _rpc_server->AddService(friend_service, brpc::ServiceOwnership::SERVER_OWNS_SERVICE);
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
        MindbniM::FriendServer::ptr newFriendServer()
        {
            if(_rpc_server==nullptr)
            {
                LOG_ROOT_ERROR<<"未初始化rpc服务器";
                abort();
            }
            return std::make_shared<FriendServer>(_reg_client, _rpc_server);
        }

    private:
        Registry::ptr _reg_client;
        Discovery::ptr _discover;
        std::shared_ptr<brpc::Server> _rpc_server;
        std::shared_ptr<odb::mysql::database> _mysql;
        std::shared_ptr<elasticlient::Client> _es;
        ServiceManager::ptr _service_manager;
    };
};