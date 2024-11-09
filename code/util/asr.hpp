#pragma once
#include"log.hpp"
#include"aipcpp/speech.h"
#include<jsoncpp/json/json.h>
namespace MindbniM
{
    class ASRClient
    {
    public:
        ASRClient(const std::string& id,const std::string& aip_key,const std::string& secert_key):_client(id,aip_key,secert_key)
        {}
        std::string asr(const std::string&file)
        {
            std::string file_ctl;
            aip::get_file_content(file.c_str(),&file_ctl);
            Json::Value result=_client.recognize(file_ctl,"pcm",16000,aip::null);
            if(result["err_no"].asInt()!=0)
            {
                LOG_ROOT_ERROR<<"语言识别失败"<<result["err_msg"].asString();
                return "";
            }
            else return result["result"][0].asString();
        }
    private:
        aip::Speech _client;
    };

}