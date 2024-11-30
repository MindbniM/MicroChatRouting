#pragma once
#define GET_PHONE_VERIFY_CODE "/service/user/get_phone_verify_code"             //获取短信验证码
#define USERNAME_REGISTER "/service/user/username_register"                     //用户名密码注册
#define USERNAME_LOGIN "/service/user/username_login"                           //用户名密码登录
#define PHONE_REGISTER "/service/user/phone_register"                           //手机号码注册
#define PHONE_LOGIN "/service/user/phone_login"                                 //手机号码登录
#define GET_USER_INFO "/service/user/get_user_info"                             //获取个人信息
#define SET_AVATAR "/service/user/set_avatar"                                   //修改头像
#define SET_NICKNAME "/service/user/set_nickname"                               //修改昵称
#define SET_DESCRIPTION "/service/user/set_description"                         //修改签名
#define SET_PHONE "/service/user/set_phone"                                     //修改绑定手机
#define GET_FRIEND_LIST "/service/friend/get_friend_list"                       //获取好友列表
#define GET_FRIEND_INFO "/service/friend/get_friend_info"                       //获取好友信息
#define ADD_FRIEND_APPLY "/service/friend/add_friend_apply"                     //发送好友申请
#define ADD_FRIEND_PROCESS "/service/friend/add_friend_process"                 //好友申请处理
#define REMOVE_FRIEND "/service/friend/remove_friend"                           //删除好友
#define SEARCH_FRIEND "/service/friend/search_friend"                           //搜索用户
#define GET_CHAT_SESSION_LIST "/service/friend/get_chat_session_list"           //获取指定用户的消息会话列表
#define CREATE_CHAT_SESSION "/service/friend/create_chat_session"               //创建消息会话
#define GET_CHAT_SESSION_MEMBER "/service/friend/get_chat_session_member"       //获取消息会话成员列表
#define GET_PENDING_FRIEND_EV "/service/friend/get_pending_friend_events"       //获取待处理好友申请事件列表
#define GET_HISTORY "/service/message_storage/get_history"                      //获取历史消息/离线消息列表
#define GET_RECENT "/service/message_storage/get_recent"                        //获取最近 N 条消息列表
#define SEARCH_HISTORY "/service/message_storage/search_history"                //搜索历史消息
#define NEW_MESSAGE "/service/message_transmit/new_message"                     //发送消息
#define GET_SINGLE_FILE "/service/file/get_single_file"                         //获取单个文件数据
#define GET_MULTI_FILE "/service/file/get_multi_file"                           //获取多个文件数据
#define PUT_SINGLE_FILE "/service/file/put_single_file"                         //发送单个文件
#define PUT_MULTI_FILE "/service/file/put_multi_file"                           //发送多个文件
#define RECOGNITION "/service/speech/recognition"                               //语音转文字

#define USER_SERVICE "/service/user_service"
#define FRIEND_SERVICE "/service/friend_service"
#define FILE_SERVICE "/service/file_service"
#define MESSAGE_SERVICE "/service/message_storage"
#define SPEECH_SERVICE "/service/speech_service"
#define TRANSMIT_SERVICE "/service/transmite_service" 