#pragma once
#include <QString>
#include <QIcon>
#include <QUuid>
#include <QDateTime>
#include <QPixmap>
#include <QFile>
#include <QFileInfo>
namespace model {

#define TAG QString("[%1][%2: %3]").arg(model::formatTime(model::getTime()),model::getFileName(__FILE__),QString::number(__LINE__))

#define LOG() qDebug().noquote()<< TAG

/**
 * @brief 提取文件名
 * @param path 路径
 */
static inline QString getFileName(const QString& path)
{
    QFileInfo file(path);
    return file.fileName();
}
/**
 * @brief 获取当前秒级别时间戳
 */
static inline int64_t getTime()
{
    return QDateTime::currentSecsSinceEpoch();
}

/**
 * @brief 格式化时间戳
 * @param time 秒级别时间戳
 */
static inline QString formatTime(int64_t time)
{
    QDateTime data=QDateTime::fromSecsSinceEpoch(time);
    return data.toString("MM-dd HH:mm:ss");
}

/**
 * @brief 构建QIcon对象
 * @param content 数据
 */
static inline QIcon makeIcon(const QByteArray& content)
{
    QPixmap p;
    p.loadFromData(content);
    return p;
}

/**
 * @brief 从文件读取
 * @param path 路径
 */
static inline QByteArray loadFileToByteArray(const QString& path)
{
    QFile file(path);
    bool ret=file.open(QFile::ReadOnly);
    if(!ret)
    {
        qDebug()<<"文件打开失败";
        return QByteArray();
    }
    return file.readAll();
}

/**
 * @brief 写文件
 * @param path 路径
 * @param content 内容
 */
static inline void writeFileToByteArray(const QString& path,const QByteArray& content)
{
    QFile file(path);
    bool ret=file.open(QFile::WriteOnly);
    if(!ret)
    {
        qDebug()<<"文件打开失败";
        return ;
    }
    file.write(content);
    file.flush();
    file.close();
}


/**
 * @brief 用户信息
 */
class UserInfo {
    QString userId = "";			// 用户编号
    QString nickname = "";			// 用户昵称
    QString description = ""; 		// 用户签名
    QString phone = "";				// 手机号码
    QIcon avatar;					// 用户头像

};

/**
 * @brief 消息类型
 */
enum MessageType {
    TEXT_TYPE,		// 文本消息
    IMAGE_TYPE, 	// 图片消息
    FILE_TYPE, 		// 文件消息
    SPEECH_TYPE 	// 语音消息
};

/**
 * @brief 消息信息
 */
class Message {
public:
    QString messageId = "";				// 消息的编号
    QString chatSessionId = "";			// 消息所属会话的编号
    QString time = "";					// 消息的时间. 通过 "格式化" 时间的方式来表示. 形如 06-07 12:00:00
    MessageType messageType = TEXT_TYPE;// 消息类型
    UserInfo sender;					// 发送者的信息
    QByteArray content;					// 消息的正文内容
    QString fileId = "";				// 文件的身份标识. 当消息类型为 文件, 图片, 语音 的时候, 才有效. 当消息类型为 文本, 则为 ""
    QString fileName = ""; 				// 文件名称. 只是当消息类型为 文件 消息, 才有效. 其他消息均为 ""

    /**
     * @brief 生成消息对j象
     * @param messageType
     * @param chatSessionId
     * @param sender
     * @param content
     * @param extraInfo
     */
    static Message makeMessage(MessageType messageType, const QString& chatSessionId, const UserInfo& sender,
                               const QByteArray& content, const QString& extraInfo="")
    {
        switch(messageType)
        {
        case TEXT_TYPE:
            return makeTextMessage(chatSessionId, sender, content);
        case IMAGE_TYPE:
            return makeImageMessage(chatSessionId, sender, content);
        case FILE_TYPE:
            return makeFileMessage(chatSessionId, sender, content, extraInfo);
        case SPEECH_TYPE:
            return makeSpeechMessage(chatSessionId, sender, content);
        default:
            return Message();
        }
    }
    static QString UUID()
    {
        return "M-"+QUuid::createUuid().toString().sliced(25,12);
    }
private:
    static Message makeTextMessage(const QString& chatSessionId, const UserInfo& sender, const QByteArray& content)
    {
        Message message;
        message.messageId = QUuid::createUuid().toString();
        message.chatSessionId = chatSessionId;
        message.sender = sender;
        message.time = formatTime(getTime()); // 生成一个格式化时间
        message.content = content;
        message.messageType = TEXT_TYPE;
        return message;
    }
    static Message makeImageMessage(const QString& chatSessionId, const UserInfo& sender, const QByteArray& content)
    {
        Message message;
        message.messageId = QUuid::createUuid().toString();
        message.chatSessionId = chatSessionId;
        message.sender = sender;
        message.time = formatTime(getTime());
        message.content = content;
        message.messageType = IMAGE_TYPE;
        return message;
    }
    static Message makeFileMessage(const QString& chatSessionId, const UserInfo& sender,
                                   const QByteArray& content, const QString& filename)
    {
        Message message;
        message.messageId = QUuid::createUuid().toString();
        message.chatSessionId = chatSessionId;
        message.sender = sender;
        message.time = formatTime(getTime());
        message.content = content;
        message.messageType = FILE_TYPE;
        message.fileName=filename;
        return message;
    }
    static Message makeSpeechMessage(const QString& chatSessionId, const UserInfo& sender, const QByteArray& content)
    {
        Message message;
        message.messageId = QUuid::createUuid().toString();
        message.chatSessionId = chatSessionId;
        message.sender = sender;
        message.time = formatTime(getTime());
        message.content = content;
        message.messageType = SPEECH_TYPE;
        return message;
    }

};

/**
 * @brief 会话信息
 */
struct ChatSessionInfo {
    QString chatSessionId = "";	  		// 会话编号
    QString chatSessionName = "";		// 会话名字, 如果会话是单聊, 名字就是对方的昵称; 如果是群聊, 名字就是群聊的名称.
    Message lastMessage;				// 表示最新的消息.
    QIcon avatar;						// 会话头像. 如果会话是单聊, 头像就是对方的头像; 如果是群聊, 头像群聊的头像.
    QString userId = "";				// 对于单聊来说, 表示对方的用户 id, 对于群聊设为 ""
};


} // end model
