#pragma once
#include<json/json.h>
#include<memory>
#include<sstream>
namespace MindbniM
{
    class JSON
    {
    public:
        bool static Serializa(const Json::Value& root,std::string& str)
        {
            Json::StreamWriterBuilder wb;
            std::unique_ptr<Json::StreamWriter> w(wb.newStreamWriter());
            std::stringstream os;
            bool ret=w->write(root,&os);
            if(!ret) return false;
            str=os.str();
            return true;
        }
        bool static UnSerializa(const std::string& str,Json::Value& root)
        {
            Json::CharReaderBuilder rb;
            std::unique_ptr<Json::CharReader> r(rb.newCharReader());
            std::string err;
            bool ret=r->parse(str.c_str(),str.c_str()+str.size(),&root,&err);
            if(!ret) return false;
            return true;
        }
        
    };
}
