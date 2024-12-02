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
            rsp.set_success(false);
            rsp.set_errmsg(error);
            response.set_content(rsp.SerializeAsString(),"application/x-protbuf");
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





namespace MindbniM
{
    GatewayServer::GatewayServer(int websocket_port,int http_port,std::shared_ptr<sw::redis::Redis> redis,ServiceManager::ptr sm)
    {
        _session = std::make_shared<Session>(redis);
        _status = std::make_shared<Status>(redis);
        _sm = sm;

        _ws_server.set_access_channels(websocketpp::log::alevel::none);
        _ws_server.init_asio();
        _ws_server.set_open_handler(std::bind(&GatewayServer::onOpen, this, std::placeholders::_1));
        _ws_server.set_close_handler(std::bind(&GatewayServer::onClose, this, std::placeholders::_1));
        auto wscb = std::bind(&GatewayServer::onMessage, this, std::placeholders::_1, std::placeholders::_2);
        _ws_server.set_message_handler(wscb);
        _ws_server.set_reuse_addr(true);
        _ws_server.listen(websocket_port);
        _ws_server.start_accept();
        _http_server.Post(GET_PHONE_VERIFY_CODE  , (httplib::Server::Handler)std::bind(&GatewayServer::GetPhoneVerifyCode         , this, std::placeholders::_1, std::placeholders::_2));
        _http_server.Post(USERNAME_REGISTER      , (httplib::Server::Handler)std::bind(&GatewayServer::UserRegister               , this, std::placeholders::_1, std::placeholders::_2));
        _http_server.Post(USERNAME_LOGIN         , (httplib::Server::Handler)std::bind(&GatewayServer::UserLogin                  , this, std::placeholders::_1, std::placeholders::_2));
        _http_server.Post(PHONE_REGISTER         , (httplib::Server::Handler)std::bind(&GatewayServer::PhoneRegister              , this, std::placeholders::_1, std::placeholders::_2));
        _http_server.Post(PHONE_LOGIN            , (httplib::Server::Handler)std::bind(&GatewayServer::PhoneLogin                 , this, std::placeholders::_1, std::placeholders::_2));
        _http_server.Post(GET_USER_INFO          , (httplib::Server::Handler)std::bind(&GatewayServer::GetUserInfo                , this, std::placeholders::_1, std::placeholders::_2));
        _http_server.Post(SET_AVATAR             , (httplib::Server::Handler)std::bind(&GatewayServer::SetUserAvatar              , this, std::placeholders::_1, std::placeholders::_2));
        _http_server.Post(SET_NICKNAME           , (httplib::Server::Handler)std::bind(&GatewayServer::SetUserNickname            , this, std::placeholders::_1, std::placeholders::_2));
        _http_server.Post(SET_DESCRIPTION        , (httplib::Server::Handler)std::bind(&GatewayServer::SetUserDescription         , this, std::placeholders::_1, std::placeholders::_2));
        _http_server.Post(SET_PHONE              , (httplib::Server::Handler)std::bind(&GatewayServer::SetUserPhoneNumber         , this, std::placeholders::_1, std::placeholders::_2));
        _http_server.Post(GET_FRIEND_LIST        , (httplib::Server::Handler)std::bind(&GatewayServer::GetFriendList              , this, std::placeholders::_1, std::placeholders::_2));
        _http_server.Post(ADD_FRIEND_APPLY       , (httplib::Server::Handler)std::bind(&GatewayServer::FriendAdd                  , this, std::placeholders::_1, std::placeholders::_2));
        _http_server.Post(ADD_FRIEND_PROCESS     , (httplib::Server::Handler)std::bind(&GatewayServer::FriendAddProcess           , this, std::placeholders::_1, std::placeholders::_2));
        _http_server.Post(REMOVE_FRIEND          , (httplib::Server::Handler)std::bind(&GatewayServer::FriendRemove               , this, std::placeholders::_1, std::placeholders::_2));
        _http_server.Post(SEARCH_FRIEND          , (httplib::Server::Handler)std::bind(&GatewayServer::FriendSearch               , this, std::placeholders::_1, std::placeholders::_2));
        _http_server.Post(GET_PENDING_FRIEND_EV  , (httplib::Server::Handler)std::bind(&GatewayServer::GetPendingFriendEventList  , this, std::placeholders::_1, std::placeholders::_2));
        _http_server.Post(GET_CHAT_SESSION_LIST  , (httplib::Server::Handler)std::bind(&GatewayServer::GetChatSessionList         , this, std::placeholders::_1, std::placeholders::_2));
        _http_server.Post(CREATE_CHAT_SESSION    , (httplib::Server::Handler)std::bind(&GatewayServer::ChatSessionCreate          , this, std::placeholders::_1, std::placeholders::_2));
        _http_server.Post(GET_CHAT_SESSION_MEMBER, (httplib::Server::Handler)std::bind(&GatewayServer::GetChatSessionMember       , this, std::placeholders::_1, std::placeholders::_2));
        _http_server.Post(GET_HISTORY            , (httplib::Server::Handler)std::bind(&GatewayServer::GetHistoryMsg              , this, std::placeholders::_1, std::placeholders::_2));
        _http_server.Post(GET_RECENT             , (httplib::Server::Handler)std::bind(&GatewayServer::GetRecentMsg               , this, std::placeholders::_1, std::placeholders::_2));
        _http_server.Post(SEARCH_HISTORY         , (httplib::Server::Handler)std::bind(&GatewayServer::MsgSearch                  , this, std::placeholders::_1, std::placeholders::_2));
        _http_server.Post(NEW_MESSAGE            , (httplib::Server::Handler)std::bind(&GatewayServer::GetTransmitTarget          , this, std::placeholders::_1, std::placeholders::_2));
        _http_server.Post(GET_SINGLE_FILE        , (httplib::Server::Handler)std::bind(&GatewayServer::GetSingleFile              , this, std::placeholders::_1, std::placeholders::_2));
        _http_server.Post(GET_MULTI_FILE         , (httplib::Server::Handler)std::bind(&GatewayServer::GetMultiFile               , this, std::placeholders::_1, std::placeholders::_2));
        _http_server.Post(PUT_SINGLE_FILE        , (httplib::Server::Handler)std::bind(&GatewayServer::PutSingleFile              , this, std::placeholders::_1, std::placeholders::_2));
        _http_server.Post(PUT_MULTI_FILE         , (httplib::Server::Handler)std::bind(&GatewayServer::PutMultiFile               , this, std::placeholders::_1, std::placeholders::_2));
        _http_server.Post(RECOGNITION            , (httplib::Server::Handler)std::bind(&GatewayServer::SpeechRecognition          , this, std::placeholders::_1, std::placeholders::_2));
        _http_thread = std::thread([this, http_port]()
        {
            _http_server.listen("0.0.0.0", http_port);
        });
        _http_thread.detach();
    }
    void GatewayServer::start()
    {
        _ws_server.run();
    }
    void GatewayServer::onOpen(websocketpp::connection_hdl hdl)
    {
        LOG_ROOT_DEBUG<<"新的长连接建立";
    }
    void GatewayServer::onClose(websocketpp::connection_hdl hdl)
    {
        LOG_ROOT_DEBUG<<"一个连接断开";
        //一个连接断开, 应该删除相应的登录会话, 登录状态, 连接信息
        auto conn = _ws_server.get_con_from_hdl(hdl);
        //获取相应的用户ID和会话ID
        Connection::ConnectionInfo info;
        if(!_connection.getinfo(conn,info))
        {
            LOG_ROOT_ERROR<<"获取连接信息失败";
            return;
        }
        //删除Status, Session, Connection
        _status->remove(info.uid);
        _session->remove(info.ssid);
        _connection.erase(conn);

    }
    void GatewayServer::onMessage(websocketpp::connection_hdl hdl, Server_t::message_ptr msg)
    {
        //建立长连接后, 客户端会发送一个消息, 包括会话ID
        auto conn = _ws_server.get_con_from_hdl(hdl);//获取连接
        ClientAuthenticationReq req;
        //1. 提取会话ID
        bool ret=req.ParseFromString(msg->get_payload());
        if(!ret)
        {
            LOG_ROOT_ERROR<<"客户端认证反序列化失败";
            _ws_server.close(hdl, websocketpp::close::status::unsupported_data, "正文反序列化失败!");
            return;
        }
        std::string ssid=req.session_id();
        //2. 验证会话ID
        std::string uid=_session->get(ssid);
        if(uid=="")
        {
            LOG_ROOT_ERROR<<"客户端认证失败, 会话ID无效";
            _ws_server.close(hdl, websocketpp::close::status::unsupported_data, "会话ID无效!");
            return;
        }
        //3. 添加管理
        _connection.insert(conn,uid,ssid);
        LOG_ROOT_DEBUG<<"客户端认证成功";
    }
    void keepAlive(Server_t::connection_ptr conn);
    void GatewayServer::SpeechRecognition(const httplib::Request& req, httplib::Response& rsp)
    {
        SpeechRecognitionReq request;
        SpeechRecognitionRsp response;
        bool ret=request.ParseFromString(req.body);
        if(!ret)
        {
            LOG_ROOT_ERROR<<"语音识别请求反序列化失败";
            return Errorctl(rsp,response,"语音识别请求反序列化失败");
        }
        std::shared_ptr<brpc::Channel> channel = _sm->choose(SPEECH_SERVICE);
        if(channel==nullptr)
        {
            LOG_ROOT_ERROR<<"语音识别服务未找到";
            return Errorctl(rsp,response,"语音识别服务未找到");
        }
        SpeechService_Stub stub(channel.get());
        brpc::Controller cntl;
        stub.SpeechRecognition(&cntl, &request, &response, nullptr);
        if(cntl.Failed())
        {
            LOG_ROOT_ERROR<<"语音识别请求失败";
            return Errorctl(rsp,response,cntl.ErrorText());
        }
        rsp.set_content(response.SerializeAsString(),"application/x-protbuf");
    }
    void GatewayServer::GetSingleFile(const httplib::Request& req, httplib::Response& rsp)
    {
        GetSingleFileReq request;
        GetSingleFileRsp response;
        bool ret = request.ParseFromString(req.body);
        if (ret == false) 
        {
            LOG_ROOT_ERROR<<"获取单个文件请求正文反序列化失败！"<<request.request_id();
            return Errorctl(rsp,response,"获取单个文件请求正文反序列化失败");
        }
        std::string ssid=request.session_id();
        std::string uid=_session->get(ssid);
        if(uid=="")
        {
            LOG_ROOT_ERROR<<"会话ID无效";
            return Errorctl(rsp,response,"会话ID无效");
        }
        request.set_user_id(uid);
        std::shared_ptr<brpc::Channel> channel = _sm->choose(FILE_SERVICE);
        if (!channel) 
        {
            LOG_ROOT_ERROR<<"文件子服务未找到！";
            return Errorctl(rsp,response,"文件子服务未找到");
        }
        FileService_Stub stub(channel.get());
        brpc::Controller cntl;
        stub.GetSingleFile(&cntl, &request, &response, nullptr);
        if (cntl.Failed()) 
        {
            LOG_ROOT_ERROR<<"文件子服务调用失败！"<<cntl.ErrorText();
            return Errorctl(rsp,response,cntl.ErrorText());
        }
        rsp.set_content(response.SerializeAsString(), "application/x-protbuf");
    }
    void GatewayServer::GetMultiFile(const httplib::Request& req, httplib::Response& rsp)
    {
        GetMultiFileReq request;
        GetMultiFileRsp response;
        bool ret = request.ParseFromString(req.body);
        if (ret == false) 
        {
            LOG_ROOT_ERROR<<"获取多个文件请求正文反序列化失败！"<<request.request_id();
            return Errorctl(rsp,response,"获取多个文件请求正文反序列化失败");
        }
        std::string ssid=request.session_id();
        std::string uid=_session->get(ssid);
        if(uid=="")
        {
            LOG_ROOT_ERROR<<"会话ID无效";
            return Errorctl(rsp,response,"会话ID无效");
        }
        request.set_user_id(uid);
        std::shared_ptr<brpc::Channel> channel = _sm->choose(FILE_SERVICE);
        if (!channel) 
        {
            LOG_ROOT_ERROR<<"文件子服务未找到！";
            return Errorctl(rsp,response,"文件子服务未找到");
        }
        FileService_Stub stub(channel.get());
        brpc::Controller cntl;
        stub.GetMultiFile(&cntl, &request, &response, nullptr);
        if (cntl.Failed()) 
        {
            LOG_ROOT_ERROR<<"文件子服务调用失败！"<<cntl.ErrorText();
            return Errorctl(rsp,response,cntl.ErrorText());
        }
        rsp.set_content(response.SerializeAsString(), "application/x-protbuf");
    }
    void GatewayServer::PutSingleFile(const httplib::Request& req, httplib::Response& rsp)
    {
        PutSingleFileReq request;
        PutSingleFileRsp response;
        bool ret = request.ParseFromString(req.body);
        if (ret == false) 
        {
            LOG_ROOT_ERROR<<"上传单个文件请求正文反序列化失败！"<<request.request_id();
            return Errorctl(rsp,response,"上传单个文件请求正文反序列化失败");
        }
        std::string ssid=request.session_id();
        std::string uid=_session->get(ssid);
        if(uid=="")
        {
            LOG_ROOT_ERROR<<"会话ID无效";
            return Errorctl(rsp,response,"会话ID无效");
        }
        request.set_user_id(uid);
        std::shared_ptr<brpc::Channel> channel = _sm->choose(FILE_SERVICE);
        if (!channel) 
        {
            LOG_ROOT_ERROR<<"文件子服务未找到！";
            return Errorctl(rsp,response,"文件子服务未找到");
        }
        FileService_Stub stub(channel.get());
        brpc::Controller cntl;
        stub.PutSingleFile(&cntl, &request, &response, nullptr);
        if (cntl.Failed()) 
        {
            LOG_ROOT_ERROR<<"文件子服务调用失败！"<<cntl.ErrorText();
            return Errorctl(rsp,response,cntl.ErrorText());
        }
        rsp.set_content(response.SerializeAsString(), "application/x-protbuf");
    }
    void GatewayServer::PutMultiFile(const httplib::Request& req, httplib::Response& rsp)
    {
        PutMultiFileReq request;
        PutMultiFileRsp response;
        bool ret = request.ParseFromString(req.body);
        if (ret == false) 
        {
            LOG_ROOT_ERROR<<"上传多个文件请求正文反序列化失败！"<<request.request_id();
            return Errorctl(rsp,response,"上传多个文件请求正文反序列化失败");
        }
        std::string ssid=request.session_id();
        std::string uid=_session->get(ssid);
        if(uid=="")
        {
            LOG_ROOT_ERROR<<"会话ID无效";
            return Errorctl(rsp,response,"会话ID无效");
        }
        request.set_user_id(uid);
        std::shared_ptr<brpc::Channel> channel = _sm->choose(FILE_SERVICE);
        if (!channel) 
        {
            LOG_ROOT_ERROR<<"文件子服务未找到！";
            return Errorctl(rsp,response,"文件子服务未找到");
        }
        FileService_Stub stub(channel.get());
        brpc::Controller cntl;
        stub.PutMultiFile(&cntl, &request, &response, nullptr);
        if (cntl.Failed()) 
        {
            LOG_ROOT_ERROR<<"文件子服务调用失败！"<<cntl.ErrorText();
            return Errorctl(rsp,response,cntl.ErrorText());
        }
        rsp.set_content(response.SerializeAsString(), "application/x-protbuf");
    }
    void GatewayServer::UserRegister(const httplib::Request& req, httplib::Response& rsp)
    {
        UserRegisterReq request;
        UserRegisterRsp response;
        bool ret = request.ParseFromString(req.body);
        if (ret == false) 
        {
            LOG_ROOT_ERROR<<"用户注册请求正文反序列化失败！"<<request.request_id();
            return Errorctl(rsp,response,"用户注册请求正文反序列化失败");
        }
        std::shared_ptr<brpc::Channel> channel = _sm->choose(USER_SERVICE);
        if (!channel) 
        {
            LOG_ROOT_ERROR<<"用户子服务未找到！";
            return Errorctl(rsp,response,"用户子服务未找到");
        }
        UserService_Stub stub(channel.get());
        brpc::Controller cntl;
        stub.UserRegister(&cntl, &request, &response, nullptr);
        if (cntl.Failed()) 
        {
            LOG_ROOT_ERROR<<"用户子服务调用失败！"<<cntl.ErrorText();
            return Errorctl(rsp,response,cntl.ErrorText());
        }
        rsp.set_content(response.SerializeAsString(), "application/x-protbuf");
    }

    void GatewayServer::UserLogin(const httplib::Request& req, httplib::Response& rsp)
    {
        UserLoginReq request;
        UserLoginRsp response;
        bool ret = request.ParseFromString(req.body);
        if (ret == false) 
        {
            LOG_ROOT_ERROR<<"用户登录请求正文反序列化失败！"<<request.request_id();
            return Errorctl(rsp,response,"用户登录请求正文反序列化失败");
        }
        std::shared_ptr<brpc::Channel> channel = _sm->choose(USER_SERVICE);
        if (!channel) 
        {
            LOG_ROOT_ERROR<<"用户子服务未找到！";
            return Errorctl(rsp,response,"用户子服务未找到");
        }
        UserService_Stub stub(channel.get());
        brpc::Controller cntl;
        stub.UserLogin(&cntl, &request, &response, nullptr);
        if (cntl.Failed()) 
        {
            LOG_ROOT_ERROR<<"用户子服务调用失败！"<<cntl.ErrorText();
            return Errorctl(rsp,response,cntl.ErrorText());
        }
        rsp.set_content(response.SerializeAsString(), "application/x-protbuf");
    }
    void GatewayServer::GetPhoneVerifyCode(const httplib::Request& req, httplib::Response& rsp)
    {
        PhoneVerifyCodeReq request;
        PhoneVerifyCodeRsp response;
        bool ret = request.ParseFromString(req.body);
        if (ret == false) 
        {
            LOG_ROOT_ERROR<<"获取短信验证码请求正文反序列化失败！"<<request.request_id();
            return Errorctl(rsp,response,"获取短信验证码请求正文反序列化失败");
        }
        std::shared_ptr<brpc::Channel> channel = _sm->choose(USER_SERVICE);
        if (!channel) 
        {
            LOG_ROOT_ERROR<<"用户子服务未找到！";
            return Errorctl(rsp,response,"用户子服务未找到");
        }
        UserService_Stub stub(channel.get());
        brpc::Controller cntl;
        stub.GetPhoneVerifyCode(&cntl, &request, &response, nullptr);
        if (cntl.Failed()) 
        {
            LOG_ROOT_ERROR<<"用户子服务调用失败！"<<cntl.ErrorText();
            return Errorctl(rsp,response,cntl.ErrorText());
        }
        rsp.set_content(response.SerializeAsString(), "application/x-protbuf");
    }
    void GatewayServer::PhoneRegister(const httplib::Request& req, httplib::Response& rsp)
    {
        PhoneRegisterReq request;
        PhoneRegisterRsp response;
        bool ret = request.ParseFromString(req.body);
        if (ret == false) 
        {
            LOG_ROOT_ERROR<<"手机号注册请求正文反序列化失败！"<<request.request_id();
            return Errorctl(rsp,response,"手机号注册请求正文反序列化失败");
        }
        std::shared_ptr<brpc::Channel> channel = _sm->choose(USER_SERVICE);
        if (!channel) 
        {
            LOG_ROOT_ERROR<<"用户子服务未找到！";
            return Errorctl(rsp,response,"用户子服务未找到");
        }
        UserService_Stub stub(channel.get());
        brpc::Controller cntl;
        stub.PhoneRegister(&cntl, &request, &response, nullptr);
        if (cntl.Failed()) 
        {
            LOG_ROOT_ERROR<<"用户子服务调用失败！"<<cntl.ErrorText();
            return Errorctl(rsp,response,cntl.ErrorText());
        }
        rsp.set_content(response.SerializeAsString(), "application/x-protbuf");
    }
    void GatewayServer::PhoneLogin(const httplib::Request& req, httplib::Response& rsp)
    {
        PhoneLoginReq request;
        PhoneLoginRsp response;
        bool ret = request.ParseFromString(req.body);
        if (ret == false) 
        {
            LOG_ROOT_ERROR<<"手机号登录请求正文反序列化失败！"<<request.request_id();
            return Errorctl(rsp,response,"手机号登录请求正文反序列化失败");
        }
        std::shared_ptr<brpc::Channel> channel = _sm->choose(USER_SERVICE);
        if (!channel) 
        {
            LOG_ROOT_ERROR<<"用户子服务未找到！";
            return Errorctl(rsp,response,"用户子服务未找到");
        }
        UserService_Stub stub(channel.get());
        brpc::Controller cntl;
        stub.PhoneLogin(&cntl, &request, &response, nullptr);
        if (cntl.Failed()) 
        {
            LOG_ROOT_ERROR<<"用户子服务调用失败！"<<cntl.ErrorText();
            return Errorctl(rsp,response,cntl.ErrorText());
        }
        rsp.set_content(response.SerializeAsString(), "application/x-protbuf");
    }
    void GatewayServer::GetUserInfo(const httplib::Request& req, httplib::Response& rsp)
    {
        GetUserInfoReq request;
        GetUserInfoRsp response;
        bool ret = request.ParseFromString(req.body);
        if (ret == false) 
        {
            LOG_ROOT_ERROR<<"获取用户信息请求正文反序列化失败！"<<request.request_id();
            return Errorctl(rsp,response,"获取用户信息请求正文反序列化失败");
        }
        std::string ssid=request.session_id();
        std::string uid=_session->get(ssid);
        if(uid=="")
        {
            LOG_ROOT_ERROR<<"会话ID无效";
            return Errorctl(rsp,response,"会话ID无效");
        }
        request.set_user_id(uid);
        std::shared_ptr<brpc::Channel> channel = _sm->choose(USER_SERVICE);
        if (!channel) 
        {
            LOG_ROOT_ERROR<<"用户子服务未找到！";
            return Errorctl(rsp,response,"用户子服务未找到");
        }
        UserService_Stub stub(channel.get());
        brpc::Controller cntl;
        stub.GetUserInfo(&cntl, &request, &response, nullptr);
        if (cntl.Failed()) 
        {
            LOG_ROOT_ERROR<<"用户子服务调用失败！"<<cntl.ErrorText();
            return Errorctl(rsp,response,cntl.ErrorText());
        }
        rsp.set_content(response.SerializeAsString(), "application/x-protbuf");
    }
    void GatewayServer::GetMultiUserInfo(const httplib::Request& req, httplib::Response& rsp)
    {
    }
    void GatewayServer::SetUserAvatar(const httplib::Request& req, httplib::Response& rsp)
    {
        SetUserAvatarReq request;
        SetUserAvatarRsp response;
        bool ret = request.ParseFromString(req.body);
        if (ret == false) 
        {
            LOG_ROOT_ERROR<<"修改用户头像请求正文反序列化失败！"<<request.request_id();
            return Errorctl(rsp,response,"修改用户头像请求正文反序列化失败");
        }
        std::string ssid=request.session_id();
        std::string uid=_session->get(ssid);
        if(uid=="")
        {
            LOG_ROOT_ERROR<<"会话ID无效";
            return Errorctl(rsp,response,"会话ID无效");
        }
        request.set_user_id(uid);
        std::shared_ptr<brpc::Channel> channel = _sm->choose(USER_SERVICE);
        if (!channel) 
        {
            LOG_ROOT_ERROR<<"用户子服务未找到！";
            return Errorctl(rsp,response,"用户子服务未找到");
        }
        UserService_Stub stub(channel.get());
        brpc::Controller cntl;
        stub.SetUserAvatar(&cntl, &request, &response, nullptr);
        if (cntl.Failed()) 
        {
            LOG_ROOT_ERROR<<"用户子服务调用失败！"<<cntl.ErrorText();
            return Errorctl(rsp,response,cntl.ErrorText());
        }
        rsp.set_content(response.SerializeAsString(), "application/x-protbuf");
    }
    void GatewayServer::SetUserNickname(const httplib::Request& req, httplib::Response& rsp)
    {
        SetUserNicknameReq request;
        SetUserNicknameRsp response;
        bool ret = request.ParseFromString(req.body);
        if (ret == false) 
        {
            LOG_ROOT_ERROR<<"修改用户名称请求正文反序列化失败！"<<request.request_id();
            return Errorctl(rsp,response,"修改用户名称请求正文反序列化失败");
        }
        std::string ssid=request.session_id();
        std::string uid=_session->get(ssid);
        if(uid=="")
        {
            LOG_ROOT_ERROR<<"会话ID无效";
            return Errorctl(rsp,response,"会话ID无效");
        }
        request.set_user_id(uid);
        std::shared_ptr<brpc::Channel> channel = _sm->choose(USER_SERVICE);
        if (!channel) 
        {
            LOG_ROOT_ERROR<<"用户子服务未找到！";
            return Errorctl(rsp,response,"用户子服务未找到");
        }
        UserService_Stub stub(channel.get());
        brpc::Controller cntl;
        stub.SetUserNickname(&cntl, &request, &response, nullptr);
        if (cntl.Failed()) 
        {
            LOG_ROOT_ERROR<<"用户子服务调用失败！"<<cntl.ErrorText();
            return Errorctl(rsp,response,cntl.ErrorText());
        }
        rsp.set_content(response.SerializeAsString(), "application/x-protbuf");
    }
    void GatewayServer::SetUserDescription(const httplib::Request& req, httplib::Response& rsp)
    {
        SetUserDescriptionReq request;
        SetUserDescriptionRsp response;
        bool ret = request.ParseFromString(req.body);
        if (ret == false) 
        {
            LOG_ROOT_ERROR<<"修改用户签名请求正文反序列化失败！"<<request.request_id();
            return Errorctl(rsp,response,"修改用户签名请求正文反序列化失败");
        }
        std::string ssid=request.session_id();
        std::string uid=_session->get(ssid);
        if(uid=="")
        {
            LOG_ROOT_ERROR<<"会话ID无效";
            return Errorctl(rsp,response,"会话ID无效");
        }
        request.set_user_id(uid);
        std::shared_ptr<brpc::Channel> channel = _sm->choose(USER_SERVICE);
        if (!channel) 
        {
            LOG_ROOT_ERROR<<"用户子服务未找到！";
            return Errorctl(rsp,response,"用户子服务未找到");
        }
        UserService_Stub stub(channel.get());
        brpc::Controller cntl;
        stub.SetUserDescription(&cntl, &request, &response, nullptr);
        if (cntl.Failed()) 
        {
            LOG_ROOT_ERROR<<"用户子服务调用失败！"<<cntl.ErrorText();
            return Errorctl(rsp,response,cntl.ErrorText());
        }
        rsp.set_content(response.SerializeAsString(), "application/x-protbuf");
    }
    void GatewayServer::SetUserPhoneNumber(const httplib::Request& req, httplib::Response& rsp)
    {
        SetUserPhoneNumberReq request;
        SetUserPhoneNumberRsp response;
        bool ret = request.ParseFromString(req.body);
        if (ret == false) 
        {
            LOG_ROOT_ERROR<<"修改用户手机号请求正文反序列化失败！"<<request.request_id();
            return Errorctl(rsp,response,"修改用户手机号请求正文反序列化失败");
        }
        std::string ssid=request.session_id();
        std::string uid=_session->get(ssid);
        if(uid=="")
        {
            LOG_ROOT_ERROR<<"会话ID无效";
            return Errorctl(rsp,response,"会话ID无效");
        }
        request.set_user_id(uid);
        std::shared_ptr<brpc::Channel> channel = _sm->choose(USER_SERVICE);
        if (!channel) 
        {
            LOG_ROOT_ERROR<<"用户子服务未找到！";
            return Errorctl(rsp,response,"用户子服务未找到");
        }
        UserService_Stub stub(channel.get());
        brpc::Controller cntl;
        stub.SetUserPhoneNumber(&cntl, &request, &response, nullptr);
        if (cntl.Failed()) 
        {
            LOG_ROOT_ERROR<<"用户子服务调用失败！"<<cntl.ErrorText();
            return Errorctl(rsp,response,cntl.ErrorText());
        }
        rsp.set_content(response.SerializeAsString(), "application/x-protbuf");
    }
    void GatewayServer::GetTransmitTarget(const httplib::Request& req, httplib::Response& rsp)
    {
        NewMessageReq request;
        GetTransmitTargetRsp response; //消息转发子服务返回的
        NewMessageRsp ret;         //要给客户端返回的
        bool r = request.ParseFromString(req.body);
        if (!r) 
        {
            LOG_ROOT_ERROR<<"获取新消息转发列表请求正文反序列化失败！"<<request.request_id();
            return Errorctl(rsp,response,"获取新消息转发请求正文反序列化失败");
        }
        std::string ssid=request.session_id();
        std::string uid=_session->get(ssid);
        if(uid=="")
        {
            LOG_ROOT_ERROR<<"会话ID无效";
            return Errorctl(rsp,response,"会话ID无效");
        }
        request.set_user_id(uid);
        std::shared_ptr<brpc::Channel> channel = _sm->choose(TRANSMIT_SERVICE);
        if (!channel) 
        {
            LOG_ROOT_ERROR<<"消息转发子服务未找到！";
            return Errorctl(rsp,response,"消息转发子服务未找到");
        }
        MsgTransmitService_Stub stub(channel.get());
        brpc::Controller cntl;
        stub.GetTransmitTarget(&cntl, &request, &response, nullptr);
        if (cntl.Failed()) 
        {
            LOG_ROOT_ERROR<<"消息转发子服务调用失败！"<<cntl.ErrorText();
            return Errorctl(rsp,response,cntl.ErrorText());
        }
        if (response.success())
        {
            for (int i = 0; i < response.target_id_list_size(); i++) 
            {
                std::string notify_uid = response.target_id_list(i);
                if (notify_uid == uid) continue; //不通知自己
                auto conn = _connection.connection(notify_uid);
                if (!conn) continue;
                NotifyMessage notify;
                notify.set_notify_type(NotifyType::CHAT_MESSAGE_NOTIFY);
                auto msg_info = notify.mutable_new_message_info();
                msg_info->mutable_message_info()->CopyFrom(response.message());
                conn->send(notify.SerializeAsString(), websocketpp::frame::opcode::value::binary);
            }
        }
        ret.set_success(response.success());
        ret.set_request_id(request.request_id());
        ret.set_errmsg(response.errmsg());
        rsp.set_content(ret.SerializeAsString(), "application/x-protbuf");
    }
    void GatewayServer::GetHistoryMsg(const httplib::Request& req, httplib::Response& rsp)
    {
        GetHistoryMsgReq request;
        GetHistoryMsgRsp response;
        bool ret = request.ParseFromString(req.body);
        if (ret == false) 
        {
            LOG_ROOT_ERROR<<"获取历史消息请求正文反序列化失败！"<<request.request_id();
            return Errorctl(rsp,response,"获取历史消息请求正文反序列化失败");
        }
        std::string ssid=request.session_id();
        std::string uid=_session->get(ssid);
        if(uid=="")
        {
            LOG_ROOT_ERROR<<"会话ID无效";
            return Errorctl(rsp,response,"会话ID无效");
        }
        request.set_user_id(uid);
        std::shared_ptr<brpc::Channel> channel = _sm->choose(MESSAGE_SERVICE);
        if (!channel) 
        {
            LOG_ROOT_ERROR<<"消息子服务未找到！";
            return Errorctl(rsp,response,"消息子服务未找到");
        }
        MsgStorageService_Stub stub(channel.get());
        brpc::Controller cntl;
        stub.GetHistoryMsg(&cntl, &request, &response, nullptr);
        if (cntl.Failed()) 
        {
            LOG_ROOT_ERROR<<"消息子服务调用失败！"<<cntl.ErrorText();
            return Errorctl(rsp,response,cntl.ErrorText());
        }
        rsp.set_content(response.SerializeAsString(), "application/x-protbuf"); 
    }
    void GatewayServer::GetRecentMsg(const httplib::Request& req, httplib::Response& rsp)
    {
        GetRecentMsgReq request;
        GetRecentMsgRsp response;
        bool ret = request.ParseFromString(req.body);
        if (ret == false) 
        {
            LOG_ROOT_ERROR<<"获取最近消息请求正文反序列化失败！"<<request.request_id();
            return Errorctl(rsp,response,"获取最新消息请求正文反序列化失败");
        }
        std::string ssid=request.session_id();
        std::string uid=_session->get(ssid);
        if(uid=="")
        {
            LOG_ROOT_ERROR<<"会话ID无效";
            return Errorctl(rsp,response,"会话ID无效");
        }
        request.set_user_id(uid);
        std::shared_ptr<brpc::Channel> channel = _sm->choose(MESSAGE_SERVICE);
        if (!channel) 
        {
            LOG_ROOT_ERROR<<"消息子服务未找到！";
            return Errorctl(rsp,response,"消息子服务未找到");
        }
        MsgStorageService_Stub stub(channel.get());
        brpc::Controller cntl;
        stub.GetRecentMsg(&cntl, &request, &response, nullptr);
        if (cntl.Failed()) 
        {
            LOG_ROOT_ERROR<<"消息子服务调用失败！"<<cntl.ErrorText();
            return Errorctl(rsp,response,cntl.ErrorText());
        }
        rsp.set_content(response.SerializeAsString(), "application/x-protbuf");
    }
    void GatewayServer::MsgSearch(const httplib::Request& req, httplib::Response& rsp)
    {
        MsgSearchReq request;
        MsgSearchRsp response;
        bool ret = request.ParseFromString(req.body);
        if (ret == false) 
        {
            LOG_ROOT_ERROR<<"消息搜索请求正文反序列化失败！"<<request.request_id();
            return Errorctl(rsp,response,"消息搜索请求正文反序列化失败");
        }
        std::string ssid=request.session_id();
        std::string uid=_session->get(ssid);
        if(uid=="")
        {
            LOG_ROOT_ERROR<<"会话ID无效";
            return Errorctl(rsp,response,"会话ID无效");
        }
        request.set_user_id(uid);
        std::shared_ptr<brpc::Channel> channel = _sm->choose(MESSAGE_SERVICE);
        if (!channel) 
        {
            LOG_ROOT_ERROR<<"消息子服务未找到！";
            return Errorctl(rsp,response,"消息子服务未找到");
        }
        MsgStorageService_Stub stub(channel.get());
        brpc::Controller cntl;
        stub.MsgSearch(&cntl, &request, &response, nullptr);
        if (cntl.Failed()) 
        {
            LOG_ROOT_ERROR<<"消息子服务调用失败！"<<cntl.ErrorText();
            return Errorctl(rsp,response,cntl.ErrorText());
        }
        rsp.set_content(response.SerializeAsString(), "application/x-protbuf");
    }
    void GatewayServer::GetFriendList(const httplib::Request& req, httplib::Response& rsp)
    {
        GetFriendListReq request;
        GetFriendListRsp response;
        bool ret = request.ParseFromString(req.body);
        if (ret == false) 
        {
            LOG_ROOT_ERROR<<"获取好友列表请求正文反序列化失败！"<<request.request_id();
            return Errorctl(rsp,response,"获取好友列表请求正文反序列化失败");
        }
        std::string ssid=request.session_id();
        std::string uid=_session->get(ssid);
        if(uid=="")
        {
            LOG_ROOT_ERROR<<"会话ID无效";
            return Errorctl(rsp,response,"会话ID无效");
        }
        request.set_user_id(uid);
        std::shared_ptr<brpc::Channel> channel = _sm->choose(FRIEND_SERVICE);
        if (!channel) 
        {
            LOG_ROOT_ERROR<<"好友子服务未找到！";
            return Errorctl(rsp,response,"好友子服务未找到");
        }
        FriendService_Stub stub(channel.get());
        brpc::Controller cntl;
        stub.GetFriendList(&cntl, &request, &response, nullptr);
        if (cntl.Failed()) 
        {
            LOG_ROOT_ERROR<<"好友子服务调用失败！"<<cntl.ErrorText();
            return Errorctl(rsp,response,cntl.ErrorText());
        }
        rsp.set_content(response.SerializeAsString(), "application/x-protbuf");
    }
    void GatewayServer::FriendRemove(const httplib::Request& req, httplib::Response& rsp)
    {
        FriendRemoveReq request;
        FriendRemoveRsp response;
        bool ret = request.ParseFromString(req.body);
        if (ret == false) 
        {
            LOG_ROOT_ERROR<<"好友删除请求正文反序列化失败！"<<request.request_id();
            return Errorctl(rsp,response,"好友删除请求正文反序列化失败");
        }
        std::string ssid=request.session_id();
        std::string uid=_session->get(ssid);
        if(uid=="")
        {
            LOG_ROOT_ERROR<<"会话ID无效";
            return Errorctl(rsp,response,"会话ID无效");
        }
        request.set_user_id(uid);
        std::shared_ptr<brpc::Channel> channel = _sm->choose(FRIEND_SERVICE);
        if (!channel) 
        {
            LOG_ROOT_ERROR<<"好友子服务未找到！";
            return Errorctl(rsp,response,"好友子服务未找到");
        }
        FriendService_Stub stub(channel.get());
        brpc::Controller cntl;
        stub.FriendRemove(&cntl, &request, &response, nullptr);
        if (cntl.Failed()) 
        {
            LOG_ROOT_ERROR<<"好友子服务调用失败！"<<cntl.ErrorText();
            return Errorctl(rsp,response,cntl.ErrorText());
        }
        auto conn = _connection.connection(request.peer_id());
        if(response.success()&&conn)
        {
            //成功, 通知对方
            NotifyMessage msg;
            msg.set_notify_type(NotifyType::FRIEND_REMOVE_NOTIFY);
            auto result=msg.mutable_friend_remove();
            result->set_user_id(uid);
            conn->send(msg.SerializeAsString(), websocketpp::frame::opcode::binary);
        }
        rsp.set_content(response.SerializeAsString(), "application/x-protbuf");
    }
    void GatewayServer::FriendAdd(const httplib::Request& req, httplib::Response& rsp)
    {
        FriendAddReq request;
        FriendAddRsp response;
        bool ret = request.ParseFromString(req.body);
        if (ret == false) 
        {
            LOG_ROOT_ERROR<<"好友申请请求正文反序列化失败！"<<request.request_id();
            return Errorctl(rsp,response,"好友申请请求正文反序列化失败");
        }
        std::string ssid=request.session_id();
        std::string uid=_session->get(ssid);
        if(uid=="")
        {
            LOG_ROOT_ERROR<<"会话ID无效";
            return Errorctl(rsp,response,"会话ID无效");
        }
        request.set_user_id(uid);
        std::shared_ptr<brpc::Channel> channel = _sm->choose(FRIEND_SERVICE);
        if (!channel) 
        {
            LOG_ROOT_ERROR<<"好友子服务未找到！";
            return Errorctl(rsp,response,"好友子服务未找到");
        }
        FriendService_Stub stub(channel.get());
        brpc::Controller cntl;
        stub.FriendAdd(&cntl, &request, &response, nullptr);
        if (cntl.Failed()) 
        {
            LOG_ROOT_ERROR<<"好友子服务调用失败！"<<cntl.ErrorText();
            return Errorctl(rsp,response,cntl.ErrorText());
        }
        auto conn = _connection.connection(request.respondent_id());
        if(response.success()&&conn)
        {
            //成功, 通知对方
            std::shared_ptr<brpc::Channel> user_ch=_sm->choose(USER_SERVICE);
            if(!user_ch)
            {
                LOG_ROOT_ERROR<<"用户子服务未找到！";
                return Errorctl(rsp,response,"用户子服务未找到");
            }
            UserServiceClient uc(user_ch);
            UserInfo u;
            uc.GetUserInfo(uid, u);
            NotifyMessage msg;
            msg.set_notify_type(NotifyType::FRIEND_ADD_APPLY_NOTIFY);
            msg.mutable_friend_add_apply()->mutable_user_info()->CopyFrom(u);
            conn->send(msg.SerializeAsString(), websocketpp::frame::opcode::binary);
        }
        rsp.set_content(response.SerializeAsString(), "application/x-protbuf");
    }
    void GatewayServer::FriendAddProcess(const httplib::Request& req, httplib::Response& rsp)
    {
        FriendAddProcessReq request;
        FriendAddProcessRsp response;
        bool ret = request.ParseFromString(req.body);
        if (ret == false) 
        {
            LOG_ROOT_ERROR<<"好友申请处理请求正文反序列化失败！"<<request.request_id();
            return Errorctl(rsp,response,"好友申请处理请求正文反序列化失败");
        }
        std::string ssid=request.session_id();
        std::string uid=_session->get(ssid);
        if(uid=="")
        {
            LOG_ROOT_ERROR<<"会话ID无效";
            return Errorctl(rsp,response,"会话ID无效");
        }
        request.set_user_id(uid);
        std::shared_ptr<brpc::Channel> channel = _sm->choose(FRIEND_SERVICE);
        if (!channel) 
        {
            LOG_ROOT_ERROR<<"好友子服务未找到！";
            return Errorctl(rsp,response,"好友子服务未找到");
        }
        FriendService_Stub stub(channel.get());
        brpc::Controller cntl;
        stub.FriendAddProcess(&cntl, &request, &response, nullptr);
        if (cntl.Failed()) 
        {
            LOG_ROOT_ERROR<<"好友子服务调用失败！"<<cntl.ErrorText();
            return Errorctl(rsp,response,cntl.ErrorText());
        }
        //获取申请人和被申请人的连接
        if(response.success())
        {
            auto process=_connection.connection(request.user_id());    //申请人
            auto apply=_connection.connection(request.apply_user_id());//被申请人
            //获取双方用户信息;
            std::shared_ptr<brpc::Channel> process_ch=_sm->choose(USER_SERVICE);
            if(!process_ch)
            {
                LOG_ROOT_ERROR<<"用户子服务未找到！";
                return Errorctl(rsp,response,"用户子服务未找到");
            }
            UserServiceClient uc(process_ch);
            UserInfo process_user, apply_user;
            uc.GetUserInfo(request.user_id(), process_user);
            uc.GetUserInfo(request.apply_user_id(),apply_user);
            if(apply)
            {
                //无论是否同意, 都要通知申请人
                NotifyMessage msg;
                msg.set_notify_type(NotifyType::FRIEND_ADD_PROCESS_NOTIFY);
                auto result=msg.mutable_friend_process_result();
                result->set_agree(request.agree());
                result->mutable_user_info()->CopyFrom(apply_user);
                apply->send(msg.SerializeAsString(), websocketpp::frame::opcode::value::binary);
                LOG_ROOT_DEBUG<<"对申请人发送通知";
            }
            //如果同意, 通知双方会话建立
            if(request.agree()&&process)//通知申请人
            {
                NotifyMessage msg;
                msg.set_notify_type(NotifyType::CHAT_SESSION_CREATE_NOTIFY);
                auto result=msg.mutable_new_chat_session_info()->mutable_chat_session_info();
                result->set_single_chat_friend_id(request.apply_user_id());//单聊会话是对方的id
                result->set_chat_session_id(response.new_session_id());
                result->set_chat_session_name(apply_user.nickname());
                result->set_avatar(apply_user.avatar());
                process->send(msg.SerializeAsString(), websocketpp::frame::opcode::value::binary);
                LOG_ROOT_DEBUG<<"对申请人进行会话创建通知！";
            }
            if(request.agree()&&process)//通知被申请人
            {
                NotifyMessage msg;
                msg.set_notify_type(NotifyType::CHAT_SESSION_CREATE_NOTIFY);
                auto result=msg.mutable_new_chat_session_info()->mutable_chat_session_info();
                result->set_single_chat_friend_id(request.user_id());//单聊会话是对方的id
                result->set_chat_session_id(response.new_session_id());
                result->set_chat_session_name(process_user.nickname());
                result->set_avatar(process_user.avatar());
                process->send(msg.SerializeAsString(), websocketpp::frame::opcode::value::binary);
                LOG_ROOT_DEBUG<<"对被申请人进行会话创建通知！";
            }
        }
        rsp.set_content(response.SerializeAsString(), "application/x-protbuf");
    }
    void GatewayServer::FriendSearch(const httplib::Request& req, httplib::Response& rsp)
    {
        FriendSearchReq request;
        FriendSearchRsp response;
        bool ret = request.ParseFromString(req.body);
        if (ret == false) 
        {
            LOG_ROOT_ERROR<<"用户搜索请求正文反序列化失败！"<<request.request_id();
            return Errorctl(rsp,response,"用户搜索请求正文反序列化失败");
        }
        std::string ssid=request.session_id();
        std::string uid=_session->get(ssid);
        if(uid=="")
        {
            LOG_ROOT_ERROR<<"会话ID无效";
            return Errorctl(rsp,response,"会话ID无效");
        }
        request.set_user_id(uid);
        std::shared_ptr<brpc::Channel> channel = _sm->choose(FRIEND_SERVICE);
        if (!channel) 
        {
            LOG_ROOT_ERROR<<"好友子服务未找到！";
            return Errorctl(rsp,response,"好友子服务未找到");
        }
        FriendService_Stub stub(channel.get());
        brpc::Controller cntl;
        stub.FriendSearch(&cntl, &request, &response, nullptr);
        if (cntl.Failed()) 
        {
            LOG_ROOT_ERROR<<"好友子服务调用失败！"<<cntl.ErrorText();
            return Errorctl(rsp,response,cntl.ErrorText());
        }
        rsp.set_content(response.SerializeAsString(), "application/x-protbuf");
    }
    void GatewayServer::ChatSessionCreate(const httplib::Request& req, httplib::Response& rsp)
    {
        ChatSessionCreateReq request;
        ChatSessionCreateRsp response;
        bool ret = request.ParseFromString(req.body);
        if (ret == false) 
        {
            LOG_ROOT_ERROR<<"会话创建请求正文反序列化失败！"<<request.request_id();
            return Errorctl(rsp,response,"会话创建请求正文反序列化失败");
        }
        std::string ssid=request.session_id();
        std::string uid=_session->get(ssid);
        if(uid=="")
        {
            LOG_ROOT_ERROR<<"会话ID无效";
            return Errorctl(rsp,response,"会话ID无效");
        }
        request.set_user_id(uid);
        std::shared_ptr<brpc::Channel> channel = _sm->choose(FRIEND_SERVICE);
        if (!channel) 
        {
            LOG_ROOT_ERROR<<"好友子服务未找到！";
            return Errorctl(rsp,response,"好友子服务未找到");
        }
        FriendService_Stub stub(channel.get());
        brpc::Controller cntl;
        stub.ChatSessionCreate(&cntl, &request, &response, nullptr);
        if (cntl.Failed()) 
        {
            LOG_ROOT_ERROR<<"好友子服务调用失败！"<<cntl.ErrorText();
            return Errorctl(rsp,response,cntl.ErrorText());
        }
        //通知全部在线会话成员
        if(response.success())
        {
            for(int i=0;i<request.member_id_list_size();i++)
            {
                auto process=_connection.connection(request.member_id_list(i));
                if(process)
                {
                    NotifyMessage msg;
                    msg.set_notify_type(NotifyType::CHAT_SESSION_CREATE_NOTIFY);
                    auto result=msg.mutable_new_chat_session_info();
                    result->mutable_chat_session_info()->CopyFrom(response.chat_session_info());
                    process->send(msg.SerializeAsString(), websocketpp::frame::opcode::value::binary);
                    LOG_ROOT_DEBUG<<"对群聊成员"<< request.member_id_list(i)<<"进行会话创建通知";
                }
            }
        }
        response.clear_chat_session_info();
        rsp.set_content(response.SerializeAsString(), "application/x-protbuf");
    }
    void GatewayServer::GetChatSessionList(const httplib::Request& req, httplib::Response& rsp)
    {
        GetChatSessionListReq request;
        GetChatSessionListRsp response;
        bool ret = request.ParseFromString(req.body);
        if (ret == false) 
        {
            LOG_ROOT_ERROR<<"获取会话列表请求正文反序列化失败！"<<request.request_id();
            return Errorctl(rsp,response,"获取会话列表请求正文反序列化失败");
        }
        std::string ssid=request.session_id();
        std::string uid=_session->get(ssid);
        if(uid=="")
        {
            LOG_ROOT_ERROR<<"会话ID无效";
            return Errorctl(rsp,response,"会话ID无效");
        }
        request.set_user_id(uid);
        std::shared_ptr<brpc::Channel> channel = _sm->choose(FRIEND_SERVICE);
        if (!channel) 
        {
            LOG_ROOT_ERROR<<"好友子服务未找到！";
            return Errorctl(rsp,response,"好友子服务未找到");
        }
        FriendService_Stub stub(channel.get());
        brpc::Controller cntl;
        stub.GetChatSessionList(&cntl, &request, &response, nullptr);
        if (cntl.Failed()) 
        {
            LOG_ROOT_ERROR<<"好友子服务调用失败！"<<cntl.ErrorText();
            return Errorctl(rsp,response,cntl.ErrorText());
        }
        rsp.set_content(response.SerializeAsString(), "application/x-protbuf");
    }
    void GatewayServer::GetChatSessionMember(const httplib::Request& req, httplib::Response& rsp)
    {
        GetChatSessionMemberReq request;
        GetChatSessionMemberRsp response;
        bool ret = request.ParseFromString(req.body);
        if (ret == false) 
        {
            LOG_ROOT_ERROR<<"获取聊天成员请求正文反序列化失败！"<<request.request_id();
            return Errorctl(rsp,response,"获取聊天成员请求正文反序列化失败");
        }
        std::string ssid=request.session_id();
        std::string uid=_session->get(ssid);
        if(uid=="")
        {
            LOG_ROOT_ERROR<<"会话ID无效";
            return Errorctl(rsp,response,"会话ID无效");
        }
        request.set_user_id(uid);
        std::shared_ptr<brpc::Channel> channel = _sm->choose(FRIEND_SERVICE);
        if (!channel) 
        {
            LOG_ROOT_ERROR<<"好友子服务未找到！";
            return Errorctl(rsp,response,"好友子服务未找到");
        }
        FriendService_Stub stub(channel.get());
        brpc::Controller cntl;
        stub.GetChatSessionMember(&cntl, &request, &response, nullptr);
        if (cntl.Failed()) 
        {
            LOG_ROOT_ERROR<<"好友子服务调用失败！"<<cntl.ErrorText();
            return Errorctl(rsp,response,cntl.ErrorText());
        }
        rsp.set_content(response.SerializeAsString(), "application/x-protbuf");
    }
    void GatewayServer::GetPendingFriendEventList(const httplib::Request& req, httplib::Response& rsp)
    {
        GetPendingFriendEventListReq request;
        GetPendingFriendEventListRsp response;
        bool ret = request.ParseFromString(req.body);
        if (ret == false) 
        {
            LOG_ROOT_ERROR<<"获取好友请求列表请求正文反序列化失败！"<<request.request_id();
            return Errorctl(rsp,response,"获取好友请求列表请求正文反序列化失败");
        }
        std::string ssid=request.session_id();
        std::string uid=_session->get(ssid);
        if(uid=="")
        {
            LOG_ROOT_ERROR<<"会话ID无效";
            return Errorctl(rsp,response,"会话ID无效");
        }
        request.set_user_id(uid);
        std::shared_ptr<brpc::Channel> channel = _sm->choose(FRIEND_SERVICE);
        if (!channel) 
        {
            LOG_ROOT_ERROR<<"好友子服务未找到！";
            return Errorctl(rsp,response,"好友子服务未找到");
        }
        FriendService_Stub stub(channel.get());
        brpc::Controller cntl;
        stub.GetPendingFriendEventList(&cntl, &request, &response, nullptr);
        if (cntl.Failed()) 
        {
            LOG_ROOT_ERROR<<"好友子服务调用失败！"<<cntl.ErrorText();
            return Errorctl(rsp,response,cntl.ErrorText());
        }
        rsp.set_content(response.SerializeAsString(), "application/x-protbuf");
    }
}
