#pragma once
#include<sys/types.h>
#include<sys/stat.h>
#include "uuid.hpp"
#include "file.hpp"
#include "etcd.hpp"
#include "file.pb.h"
#include <brpc/server.h>
namespace MindbniM
{
    class FileServiceImpl : public FileService
    {
    public:
        FileServiceImpl(const std::string& path):_path(path)
        {
            umask(0); 
            mkdir(path.c_str(),0775);
            if(_path.back()!='/') _path.push_back('/');
        }
        void GetSingleFile(google::protobuf::RpcController *controller, const GetSingleFileReq *req, GetSingleFileRsp *rsp, google::protobuf::Closure *done)
        {
            LOG_ROOT_DEBUG<<"获取单个文件请求";
            brpc::ClosureGuard g(done);
            rsp->set_request_id(req->request_id());
            std::string id=req->file_id();
            std::string filename=_path+id;
            std::string body;
            bool flag=File::Read(filename,body);
            if(flag==false)
            {
                rsp->set_success(false);
                rsp->set_errmsg("文件获取失败");
                return;
            }
            rsp->set_success(true);
            FileDownloadData *data =rsp->mutable_file_data();
            data->set_file_id(id);
            data->set_file_content(body);
        }
        void GetMultiFile(google::protobuf::RpcController *controller, const GetMultiFileReq *req, GetMultiFileRsp *rsp, google::protobuf::Closure *done)
        {
            brpc::ClosureGuard g(done);
            rsp->set_request_id(req->request_id());
            for(int i=0;i<req->file_id_list_size();i++)
            {
                std::string id=req->file_id_list(i);
                std::string filename=_path+id;
                std::string body;
                bool flag=File::Read(filename,body);
                if(flag==false)
                {
                    rsp->set_success(false);
                    rsp->set_errmsg("文件获取失败");
                    return;
                }
                FileDownloadData data;
                data.set_file_id(id);
                data.set_file_content(body);
                rsp->mutable_file_data()->insert({id,data});
            }
            rsp->set_success(true);
        }
        void PutSingleFile(google::protobuf::RpcController *controller, const PutSingleFileReq *req, PutSingleFileRsp *rsp, google::protobuf::Closure *done)
        {
            LOG_ROOT_DEBUG<<"上传单个文件请求";
            brpc::ClosureGuard g(done);
            rsp->set_request_id(req->request_id());
            std::string id=UUID::Get();
            std::string filename=_path+id;
            LOG_ROOT_DEBUG<<"文件内容"<<req->file_data().file_content();
            int flag=File::Write(filename,req->file_data().file_content());
            if(flag==false)
            {
                rsp->set_success(false);
                rsp->set_errmsg("文件上传失败");
                return ;
            }
            rsp->set_success(true);
            FileMessageInfo* data=rsp->mutable_file_info();
            data->set_file_id(id);
            data->set_file_size(req->file_data().file_size());
            data->set_file_name(req->file_data().file_name());
        }
        void PutMultiFile(google::protobuf::RpcController *controller, const PutMultiFileReq *req, PutMultiFileRsp *rsp, google::protobuf::Closure *done)
        {
            brpc::ClosureGuard g(done);
            rsp->set_request_id(req->request_id());
            for(int i=0;i<req->file_data_size();i++)
            {
                std::string id=UUID::Get();
                std::string filename=_path+id;
                int flag=File::Write(filename,req->file_data(i).file_content());
                if(flag==false)
                {
                    rsp->set_success(false);
                    rsp->set_errmsg("文件上传失败");
                    return ;
                }
                FileMessageInfo* add=rsp->add_file_info();
                add->set_file_id(id);
                add->set_file_name(req->file_data(i).file_name());
                add->set_file_size(req->file_data(i).file_size());
            }
            rsp->set_success(true);
        }
    private:
        std::string _path;
    };
    class FileServer
    {
    public:
        using ptr = std::shared_ptr<FileServer>;
        FileServer(Registry::ptr reg, std::shared_ptr<brpc::Server> rpc) : _reg_client(reg), _rpc_server(rpc)
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
    class FileServerBuild
    {
    public:
        void make_reg_object(const std::string &reg_host, const std::string &service_name, const std::string &service_host)
        {
            _reg_client = std::make_shared<Registry>(reg_host);
            _reg_client->registry(service_name, service_host);
        }
        void make_rpc_server(int port, int timeout, int thread_num,const std::string& path="./data/")
        {
            _rpc_server = std::make_shared<brpc::Server>();
            FileServiceImpl *file_service = new FileServiceImpl(path);
            int ret = _rpc_server->AddService(file_service, brpc::ServiceOwnership::SERVER_OWNS_SERVICE);
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
        FileServer::ptr newFileServer()
        {
            if (_reg_client == nullptr)
            {
                LOG_ROOT_ERROR << "未初始化服务注册模块";
                abort();
            }
            if (_rpc_server == nullptr)
            {
                LOG_ROOT_ERROR << "未初始化rpc服务器";
                abort();
            }
            return std::make_shared<FileServer>(_reg_client, _rpc_server);
        }

    private:
        Registry::ptr _reg_client;
        std::shared_ptr<brpc::Server> _rpc_server;
    };
}