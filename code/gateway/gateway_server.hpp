#pragma once
#include"service.h"
namespace MindbniM
{
    class GatewayServer
    {
    public:
        using ptr=std::shared_ptr<GatewayServer>;
        GatewayServer(int websocket_port,int http_port,std::shared_ptr<sw::redis::Redis> redis,ServiceManager::ptr sm);
        void start();
    private:
        void Errorctl(httplib::Response& response,auto& rsp, const std::string& error)
        {
            rsp->set_success(false);
            rsp->set_errmsg(error);
            response.set_content(rsp->SerializeAsString(),"application/x-protbuf");
        }


        //各个处理都这样
        //1.  取出http请求正文，将正文进行反序列化
        //1.5 进行用户鉴权(optional)
        //2.  进行rpc调用
        //3.  将调用结构序列化到http响应
    private:
    //websocket回调处理
        //websocket服务器连接建立回调
        void onOpen(websocketpp::connection_hdl hdl);
        //websocket服务器连接关闭回调
        void onClose(websocketpp::connection_hdl hdl);
        //websocket服务器消息接收回调
        void onMessage(websocketpp::connection_hdl hdl, Server_t::message_ptr msg);
        //设备连接保活
        void keepAlive(Server_t::connection_ptr conn);
        
    private:
    //语音识别子服务
        //语音转文字
        void SpeechRecognition(const httplib::Request& req, httplib::Response& rsp);
    private:
    //文件储存子服务
        //获取单个文件
        void GetSingleFile(const httplib::Request& req, httplib::Response& rsp);
        //批量获取文件
        void GetMultiFile(const httplib::Request& req, httplib::Response& rsp);
        //上传单个文件
        void PutSingleFile(const httplib::Request& req, httplib::Response& rsp);
        //批量上传文件
        void PutMultiFile(const httplib::Request& req, httplib::Response& rsp);

    private:
    //用户管理子服务
        //用户名注册
        void UserRegister(const httplib::Request& req, httplib::Response& rsp);
        //用户名登录
        void UserLogin(const httplib::Request& req, httplib::Response& rsp);
        //获取短信验证码
        void GetPhoneVerifyCode(const httplib::Request& req, httplib::Response& rsp);
        //手机号码注册
        void PhoneRegister(const httplib::Request& req, httplib::Response& rsp);
        //手机号码登录
        void PhoneLogin(const httplib::Request& req, httplib::Response& rsp);
        //获取用户信息
        void GetUserInfo(const httplib::Request& req, httplib::Response& rsp);
        //批量获取用户信息
        void GetMultiUserInfo(const httplib::Request& req, httplib::Response& rsp);
        //修改头像
        void SetUserAvatar(const httplib::Request& req, httplib::Response& rsp);
        //修改昵称
        void SetUserNickname(const httplib::Request& req, httplib::Response& rsp);
        //修改签名
        void SetUserDescription(const httplib::Request& req, httplib::Response& rsp);
        //修改绑定手机
        void SetUserPhoneNumber(const httplib::Request& req, httplib::Response& rsp);

    private:
    //消息转发子服务
        //获取转发列表
        void GetTransmitTarget(const httplib::Request& req, httplib::Response& rsp);
    private:
    //消息储存子服务
        //获取一个时间段的消息
        void GetHistoryMsg(const httplib::Request& req, httplib::Response& rsp);
        //获取最短的N条消息
        void GetRecentMsg(const httplib::Request& req, httplib::Response& rsp);
        //搜索历史消息
        void MsgSearch(const httplib::Request& req, httplib::Response& rsp);

    private:
    //好友管理子服务
        //获取好友信息列表
        void GetFriendList(const httplib::Request& req, httplib::Response& rsp);
        //删除好友
        void FriendRemove(const httplib::Request& req, httplib::Response& rsp);
        //发起好友申请
        void FriendAdd(const httplib::Request& req, httplib::Response& rsp);
        //处理好友申请
        void FriendAddProcess(const httplib::Request& req, httplib::Response& rsp);
        //搜索用户
        void FriendSearch(const httplib::Request& req, httplib::Response& rsp);
        //创建群聊会话
        void ChatSessionCreate(const httplib::Request& req, httplib::Response& rsp);
        //获取群聊信息列表
        void GetChatSessionList(const httplib::Request& req, httplib::Response& rsp);
        //获取群聊成员列表
        void GetChatSessionMember(const httplib::Request& req, httplib::Response& rsp);
        //获取待处理好友事件列表
        void GetPendingFriendEventList(const httplib::Request& req, httplib::Response& rsp);

    private:
        Connection _connection;
        httplib::Server _http_server;
        Server_t _ws_server;
        Session::ptr _session;
        Status::ptr _status;
        ServiceManager::ptr _sm;
        std::thread _http_thread;
    };

    class GatewayServerBuild
    {
    public:
        void make_dis_object(const std::string &dis_host, const std::string& base_dir)
        {
            _service_manager = std::make_shared<ServiceManager>();
            _service_manager->add_concern(USER_SERVICE);
            _service_manager->add_concern(FRIEND_SERVICE);
            _service_manager->add_concern(FILE_SERVICE);
            _service_manager->add_concern(MESSAGE_SERVICE);
            _service_manager->add_concern(SPEECH_SERVICE);
            _service_manager->add_concern(TRANSMIT_SERVICE);
            auto put_cb = std::bind(&ServiceManager::onServiceOnline, _service_manager.get(), std::placeholders::_1, std::placeholders::_2);
            auto del_cb = std::bind(&ServiceManager::onServiceOffline, _service_manager.get(), std::placeholders::_1, std::placeholders::_2);
            _discover= std::make_shared<Discovery>(dis_host, put_cb, del_cb);
            _discover->discover(base_dir);
        }
        void make_redis_object(const std::string &host, int port, int db)
        {
            _redis = RedisFactory::create(host, port, db);
        }
        GatewayServer::ptr newGatewayServer(int websocket_port, int http_port)
        {
            if(_discover==nullptr)
            {
                LOG_ROOT_ERROR<<"未初始化服务发现模块";
                abort();
            }
            if(_redis==nullptr)
            {
                LOG_ROOT_ERROR<<"未创建Redis模块";
                abort();
            }
            if(_service_manager==nullptr)
            {
                LOG_ROOT_ERROR<<"未初始化服务管理模块";
                abort();
            }
            return std::make_shared<GatewayServer>(websocket_port, http_port, _redis, _service_manager);
        }
    private:
        Discovery::ptr _discover;
        std::shared_ptr<sw::redis::Redis> _redis;
        ServiceManager::ptr _service_manager;
    };
}