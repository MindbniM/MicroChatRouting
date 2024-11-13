#pragma once
#include"log.hpp"
#include"asr.hpp"
#include"etcd.hpp"
#include<brpc/server.h>
namespace MindbniM
{
    class SpeechServiceImpl : public SpeechService
    {
    public:
        SpeechServiceImpl()
        {
            _client=std::make_shared<ASRClient>(getenv("SPEECH_APPID"),getenv("SPEECH_API_KEY"),getenv("SPEECH_SECRET_KEY"));
        }
        void SpeechRecognition(google::protobuf::RpcController* controller,const SpeechRecognitionReq* request,SpeechRecognitionRsp response,google::protobuf::Closure* done)
        {
            brpc::ClosureGuard guard(done);
            std::string err;
            std::string res=_client->asr(request->speech_content(),err);
            response->set_request_id(request->request_id());
            if(!err.empty())
            {
                LOG_ROOT_ERROR<<request->request_id()<<" 语音识别失败: "<<err;
                response->set_success(false);
                response->set_errmsg(err);
                return ;
            }
            response->set_success(true);
            response->set_recognition_result(res);
        }
    private:
        ASRClient::ptr _client;
    };
    class SpeechServer
    {
    public:
        using ptr=std::shared_ptr<SpeechServer>;
        SpeechServer(Registry::ptr reg,std::shared_ptr<brpc::Server> rpc):_reg_client(reg),_rpc_server(rpc)
        {}
        void start()
        {
            _rpc_server->RunUntilAskedToQuit();
        }
    private:
        Registry::ptr _reg_client;
        std::shared_ptr<brpc::Server> _rpc_server;
    };
    class SpeechServerBuild
    {
    public:
        void make_reg_object(const std::string& reg_host,const std::string& service_name,const std::string& service_host)
        {
            _reg_client=std::make_shared<Registry>(reg_host);
            _reg_client->registry(service_name,service_host);
        }
        void make_rpc_server(uint16_t port,int timeout,int thread_num)
        {
            _rpc_server=std::make_shared<brpc::Server>();
            SpeechServiceImpl* speech_service=new SpeechServiceImpl;
            int ret=_rpc_server->AddService(speech_service,brpc::ServiceOwnership::SERVER_OWNS_SERVICE);
            if(ret<0)
            {
                LOG_ROOT_ERROR<<"添加rpc服务失败";
                return ;
            }
            brpc::ServerOptions op;
            op.idle_timeout_sec=timeout;
            op.num_threads=thread_num;
            ret=_rpc_server->Start(port,&op);
            if(ret<0)
            {
                LOG_ROOT_ERROR<<"rpc服务启动失败";
                return ;
            }
        }
        SpeechServer::ptr newSpeechServer()
        {
            if(_reg_client==nullptr)
            {
                LOG_ROOT_ERROR<<"未初始化服务注册模块";
                abort();
            }
            if(_rpc_server==nullptr)
            {
                LOG_ROOT_ERROR<<"未初始化rpc服务器";
                abort();
            }
            return std::make_shared<SpeechServer>(_reg_client,_rpc_server);
        }
    private:
        Registry::ptr _reg_client;
        std::shared_ptr<brpc::Server> _rpc_server;
    };
};