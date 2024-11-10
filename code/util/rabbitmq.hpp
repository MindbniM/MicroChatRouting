#pragma oncee
#include<ev.h>
#include<amqpcpp.h>
#include<amqpcpp/libev.h>
#include<openssl/ssl.h>
#include<openssl/opensslv.h>
#include"log.hpp"
namespace MindbniM
{
    const static std::string ROUT_DEFAULT="rout_default";
    const static std::string TAG_DEFAULT="tag_default";
    class MQClient
    {
    public:
        using MessageCallBack=std::function<void(const std::string&)>;
        MQClient(const std::string& user,const std::string& password,const std::string& ip)
        {
            _loop=EV_DEFAULT;
            _headler=std::make_unique<AMQP::LibEvHandler>(_loop);
            std::string url="amqp://"+user+":"+password+"@"+ip+"/";
            AMQP::Address addr(url);
            _con=std::make_unique<AMQP::TcpConnection>(_headler.get(),addr);
            _channel=std::make_unique<AMQP::TcpChannel>(_con.get());
        }
        ~MQClient()
        {
            struct ev_async async;
            ev_async_init(&async,&WatchCallBack);
            ev_async_start(_loop,&async);
            ev_async_send(_loop,&async);
            _run.join();
            ev_loop_destroy(_loop);
        }
        void declareComponents(const std::string& exchange,const std::string& queue,const std::string& routing_key=ROUT_DEFAULT,AMQP::ExchangeType type=AMQP::ExchangeType::direct)
        {
            _channel->declareExchange(exchange,type).onError([exchange](const char* message){LOG_ROOT_ERROR<<exchange<<"交换机创建失败";exit(0);}).onSuccess([exchange](){LOG_ROOT_INFO<<exchange<<"交换机创建成功";});
            //声明队列
            _channel->declareQueue(queue).onError([queue](const char* message){LOG_ROOT_ERROR<<queue<<"队列创建失败";exit(0);}).onSuccess([queue](){LOG_ROOT_INFO<<queue<<"队列创建成功";});
            //绑定交换机和队列
            _channel->bindQueue(exchange,queue,routing_key).onError([exchange,queue](const char* message){LOG_ROOT_ERROR<<exchange<<" "<<queue<<"绑定失败";exit(0);}).onSuccess([exchange,queue](){LOG_ROOT_INFO<<exchange<<" "<<queue<<"绑定成功";});
            _run = std::thread([this]() 
            { 
                LOG_ROOT_INFO << "Starting event loop";
                ev_run(_loop); 
                LOG_ROOT_INFO << "Event loop stopped";
            });
        }
        bool publish(const std::string& exchange,const std::string& mess,const std::string& routing_key=ROUT_DEFAULT)
        {
            bool ret=_channel->publish(exchange,routing_key,mess);
            if(!ret)
            {
                LOG_ROOT_ERROR<<exchange<<" publish失败";
            }
            return ret;
        }
        void consume(const std::string& queue,const MessageCallBack& callback,const std::string& tag=TAG_DEFAULT)
        {
            _callback=callback;
            auto call=std::bind(&MQClient::CallBack,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3);
            _channel->consume(queue,tag).onReceived(call).onError([queue](const char* message){LOG_ROOT_ERROR<<queue<<"订阅失败";exit(0);}).onSuccess([queue](){LOG_ROOT_INFO<<queue<<"订阅成功";});
        }
    private:
        void static WatchCallBack(struct ev_loop* loop,struct ev_async* async,int n)
        {
            ev_break(loop,EVBREAK_ALL);
        }
        void CallBack(const AMQP::Message& message,uint64_t tag, bool re)
        {
            std::string body;
            body.assign(message.body(),message.bodySize());
            _callback(body);
            _channel->ack(tag);
        }
        struct ev_loop* _loop;
        std::unique_ptr<AMQP::LibEvHandler> _headler;
        std::unique_ptr<AMQP::TcpConnection>_con;
        std::unique_ptr<AMQP::TcpChannel> _channel;
        MessageCallBack _callback;
        std::thread _run;
    };
}
