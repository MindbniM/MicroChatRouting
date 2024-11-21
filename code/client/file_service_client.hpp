#pragma once
#include"uuid.hpp"
#include"channel.hpp"
#include"file.pb.h"
namespace MindbniM
{
    class FileServiceClient
    {
    public:
        FileServiceClient(std::shared_ptr<brpc::Channel> channel) : _channel(channel)
        {}
        bool PutSingleFile(const std::string& filename,const std::string& file_content,std::string& file_id)
        {
            brpc::Controller cntl;
            std::string request_id=UUID::Get();
            PutSingleFileReq req;
            PutSingleFileRsp rsp;
            req.set_request_id(request_id);
            FileUploadData* file_data=req.mutable_file_data();
            file_data->set_file_name(filename);
            file_data->set_file_content(file_content);
            file_data->set_file_size(file_content.size());
            FileService_Stub stub(_channel.get());
            stub.PutSingleFile(&cntl,&req,&rsp,nullptr);
            if(cntl.Failed())
            {
                LOG_ROOT_ERROR<<"文件服务器请求失败 err:"<<cntl.ErrorText();
                return false;
            }
            if(rsp.success())
            {
                LOG_ROOT_ERROR<<"文件服务器请求失败 err:"<<rsp.errmsg();
                return false;
            }
            file_id=rsp.file_info().file_id();
            return true;
        }
        bool GetSingleFile(const std::string& file_id,std::string& file_content)
        {
            brpc::Controller cntl;
            std::string request_id=UUID::Get();
            GetSingleFileReq req;
            GetSingleFileRsp rsp;
            req.set_request_id(request_id);
            req.set_file_id(file_id);
            FileService_Stub stub(_channel.get());
            stub.GetSingleFile(&cntl,&req,&rsp,nullptr);
            if(cntl.Failed())
            {
                LOG_ROOT_ERROR<<"文件服务器请求失败 err:"<<cntl.ErrorText();
                return false;
            }
            if(!rsp.success())
            {
                LOG_ROOT_ERROR<<"文件服务器请求失败 err:"<<rsp.errmsg();
                return false;
            }
            file_content=rsp.file_data().file_content();
            return true;
        }
        bool GetMultiFile(const std::vector<std::string>& file_ids,std::vector<std::string>& file_contents)
        {
            brpc::Controller cntl;
            std::string request_id=UUID::Get();
            GetMultiFileReq req;
            GetMultiFileRsp rsp;
            req.set_request_id(request_id);
            for(auto& id:file_ids)
            {
                req.add_file_id_list(id);
            }
            FileService_Stub stub(_channel.get());
            stub.GetMultiFile(&cntl,&req,&rsp,nullptr);
            if(cntl.Failed())
            {
                LOG_ROOT_ERROR<<"文件服务器请求失败 err:"<<cntl.ErrorText();
                return false;
            }
            if(!rsp.success())
            {
                LOG_ROOT_ERROR<<"文件服务器请求失败 err:"<<rsp.errmsg();
                return false;
            }
            for(auto& id:file_ids)
            {
                file_contents.emplace_back(rsp.mutable_file_data()->at(id));
            }
            return true;
        }
    private:
        std::shared_ptr<brpc::Channel> _channel;
    };
}
