#include"gateway_server.hpp"
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
    void SpeechRecognition(const httplib::Request& req, httplib::Response& rsp);
    void GetSingleFile(const httplib::Request& req, httplib::Response& rsp);
    void GetMultiFile(const httplib::Request& req, httplib::Response& rsp);
    void PutSingleFile(const httplib::Request& req, httplib::Response& rsp);
    void PutMultiFile(const httplib::Request& req, httplib::Response& rsp);
    void UserRegister(const httplib::Request& req, httplib::Response& rsp);
    void UserLogin(const httplib::Request& req, httplib::Response& rsp);
    void GetPhoneVerifyCode(const httplib::Request& req, httplib::Response& rsp);
    void PhoneRegister(const httplib::Request& req, httplib::Response& rsp);
    void PhoneLogin(const httplib::Request& req, httplib::Response& rsp);
    void GetUserInfo(const httplib::Request& req, httplib::Response& rsp);
    void GetMultiUserInfo(const httplib::Request& req, httplib::Response& rsp);
    void SetUserAvatar(const httplib::Request& req, httplib::Response& rsp);
    void SetUserNickname(const httplib::Request& req, httplib::Response& rsp);
    void SetUserDescription(const httplib::Request& req, httplib::Response& rsp);
    void SetUserPhoneNumber(const httplib::Request& req, httplib::Response& rsp);
    void GetTransmitTarget(const httplib::Request& req, httplib::Response& rsp);
    void GetHistoryMsg(const httplib::Request& req, httplib::Response& rsp);
    void GetRecentMsg(const httplib::Request& req, httplib::Response& rsp);
    void MsgSearch(const httplib::Request& req, httplib::Response& rsp);
    void GetFriendList(const httplib::Request& req, httplib::Response& rsp);
    void FriendRemove(const httplib::Request& req, httplib::Response& rsp);
    void FriendAdd(const httplib::Request& req, httplib::Response& rsp);
    void FriendAddProcess(const httplib::Request& req, httplib::Response& rsp);
    void FriendSearch(const httplib::Request& req, httplib::Response& rsp);
    void ChatSessionCreate(const httplib::Request& req, httplib::Response& rsp);
    void GetChatSessionList(const httplib::Request& req, httplib::Response& rsp);
    void GetChatSessionMember(const httplib::Request& req, httplib::Response& rsp);
    void GetPendingFriendEventList(const httplib::Request& req, httplib::Response& rsp);
}