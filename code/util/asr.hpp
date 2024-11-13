#pragma once
#include"log.hpp"
#include"aipcpp/speech.h"
#include<jsoncpp/json/json.h>
namespace MindbniM
{
    class ASRClient
    {
    public:
        using ptr=std::shared_ptr<ASRClient>;
        ASRClient(const std::string& id,const std::string& aip_key,const std::string& secert_key):_client(id,aip_key,secert_key)
        {}
        std::string asr(const std::string&data,std::string& err)
        {
            Json::Value result=_client.recognize(data,"pcm",16000,aip::null);
            if(result["err_no"].asInt()!=0)
            {
                err=result["err_msg"].asString();
                return "";
            }
            else return result["result"][0].asString();
        }
    private:
        aip::Speech _client;
    };

}