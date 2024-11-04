#pragma once
#include"../log/log.hpp"
#include"../json/json.hpp"
#include<elasticlient/client.h>
#include<cpr/response.h>
namespace MindbniM
{
    struct __cvalue__
    {
        __cvalue__(const std::string& name,const std::string& type="_doc",const std::string& id="default_index_id"):_name(name),_type(type),_id(id)
        {
            Json::Value analysis;
            Json::Value analyzer;
            Json::Value ik;
            ik["tokenizer"]="ik_max_word";
            analyzer["ik"]=ik;
            analysis["analyzer"]=analyzer;
            _settings["analysis"]=analysis;
            Json::Value root;
            root["settings"]=_settings;
            _mappings["dynamic"]=true;
        }
        __cvalue__&  append(const std::string& name,const std::string& type="text",const std::string& analyzer="ik_max_word",bool enabled=true)
        {
#define JSONXXX(root,val) root[#val]=val
            Json::Value val;
            JSONXXX(val,type);
            JSONXXX(val,analyzer);
            if(enabled==false)  JSONXXX(val,enabled);
#undef JSONXXX
            _properties[name]=val;
            return *this;
        }
        std::string get()const
        {
            Json::Value root;
            Json::Value mappings(_mappings);
            root["settings"]=_settings;
            mappings["properties"]=_properties;
            root["mappings"]=mappings;
            std::string ret;
            JSON::Serializa(root,ret);
            return ret;
        }
        std::string _name;
        std::string _type;
        std::string _id;
        Json::Value _settings;
        Json::Value _mappings;
        Json::Value _properties;
    };
    struct __ivalue__
    {
        __ivalue__(const std::string& name,const std::string& type="_doc"):_name(name),_type(type)
        {}
        template<class T>
        __ivalue__& append(const std::string& key,T&& val)
        {
            _item[key]=val;
            return *this;
        }
        std::string get() const
        {
            std::string body;
            JSON::Serializa(_item,body);
            return body;
        }
        std::string _name;
        std::string _type;
        Json::Value _item;
    };
    struct __rvalue__
    {
        __rvalue__(const std::string& name,const std::string& type, const std::string& id):_name(name),_type(type),_id(id)
        {}
        std::string _name;
        std::string _type;
        std::string _id;
    };
    struct __svalue__ 
    {
        __svalue__(const std::string &name, const std::string &type = "_doc"):_name(name), _type(type)
        {}
        __svalue__& append_must_not_terms(const std::string &key, const std::vector<std::string> &vals) 
        {
            Json::Value fields;
            for (const auto& val : vals)
            {
                fields[key].append(val);
            }
            Json::Value terms;
            terms["terms"] = fields;
            _must_not.append(terms);
            return *this;
        }
        __svalue__& append_should_match(const std::string &key, const std::string &val) 
        {
            Json::Value field;
            field[key] = val;
            Json::Value match;
            match["match"] = field;
            _should.append(match);
            return *this;
        }
        __svalue__& append_must_term(const std::string &key, const std::string &val) 
        {
            Json::Value field;
            field[key] = val;
            Json::Value term;
            term["term"] = field;
            _must.append(term);
            return *this;
        }
        __svalue__& append_must_match(const std::string &key, const std::string &val)
        {
            Json::Value field;
            field[key] = val;
            Json::Value match;
            match["match"] = field;
            _must.append(match);
            return *this;
        }
        std::string get()const
        {
            Json::Value cond;
            if (_must_not.empty() == false) cond["must_not"] = _must_not;
            if (_should.empty() == false) cond["should"] = _should;
            if (_must.empty() == false) cond["must"] = _must;
            Json::Value query;
            query["bool"] = cond;
            Json::Value root;
            root["query"] = query;

            std::string body;
            JSON::Serializa(root, body);
            return body;
        }
        std::string _name;
        std::string _type;
        Json::Value _must_not;
        Json::Value _should;
        Json::Value _must;
    };
    class ESClient
    {
    public:
        using CreateValue=__cvalue__;
        using InsertValue=__ivalue__;
        using RemoveValue=__rvalue__;
        using SearchValue=__svalue__;
        ESClient(const std::vector<std::string>& hosts={"http://127.0.0.1:9200/"}):_client(std::make_unique<elasticlient::Client>(hosts))
        {}
        bool Create(const CreateValue& cv)
        {
            std::string body=cv.get();
            if(body.empty())
            {
                LOG_ROOT_ERROR<<"ES: 创建索引"<<cv._name<<"序列化失败: ";
                return false;
            }
            try
            {
                cpr::Response re=_client->index(cv._name,cv._type,cv._id,body);
                if(re.status_code<200||re.status_code>=300)
                {
                    LOG_ROOT_ERROR<<"ES: 创建索引"<<cv._name<<"失败: "<<re.status_code;
                    return false;
                }
            }
            catch(std::exception& e)
            {
                LOG_ROOT_ERROR<<"ES: 创建索引"<<cv._name<<"失败: "<<e.what();
                return false;
            }
            LOG_ROOT_DEBUG<<"ES: 创建索引"<<cv._name<<"成功";
            return true;
        }
        bool Insert(const InsertValue& iv,const std::string& id="")
        {
            std::string body=iv.get();
            if(body.empty())
            {
                LOG_ROOT_ERROR<<"ES: 添加数据"<<iv._name<<"序列化失败: ";
                return false;
            }
            try
            {
                cpr::Response re=_client->index(iv._name,iv._type,id,body);
                if(re.status_code<200||re.status_code>=300)
                {
                    LOG_ROOT_ERROR<<"ES: 添加数据"<<iv._name<<"失败: "<<re.status_code;
                    return false;
                }
            }
            catch(std::exception& e)
            {
                LOG_ROOT_ERROR<<"ES: 添加数据"<<iv._name<<"失败: "<<e.what();
                return false;
            }
            LOG_ROOT_DEBUG<<"ES: 添加数据"<<iv._name<<"成功";
            return true;
        }
        bool Remove(const RemoveValue& rv)
        {
            try 
            {
                auto rsp = _client->remove(rv._name, rv._type, rv._id);
                if (rsp.status_code < 200 || rsp.status_code >= 300) 
                {
                    LOG_ROOT_ERROR<<"删除数据"<<rv._id<<"失败:"<<rsp.status_code;
                    return false;
                }
            }
            catch(std::exception &e) 
            {
                LOG_ROOT_ERROR<<"删除数据"<<rv._id<<"失败:"<<e.what();
                return false;
            }
            LOG_ROOT_DEBUG<<"删除数据"<<rv._id<<"成功";
            return true;
        }
        Json::Value Search(const SearchValue& sv)
        {
            std::string body=sv.get();
            if (body.empty()) 
            {
                LOG_ROOT_ERROR<<"索引序列化失败";
                return Json::Value();
            }
            //std::cout<<body<<std::endl;
            cpr::Response rsp;
            try 
            {
                rsp = _client->search(sv._name, sv._type, body);
                if (rsp.status_code < 200 || rsp.status_code >= 300) 
                {
                    LOG_ROOT_ERROR<<"检索数据"<<body<<"失败: "<<rsp.status_code;
                    return Json::Value();
                }
            }
            catch(std::exception &e) 
            {
                LOG_ROOT_ERROR<<"检索数据"<<body<<"失败: "<<e.what();
                return Json::Value();
            }
            Json::Value json_res;
            bool ret=JSON::UnSerializa(rsp.text, json_res);
            if (ret == false) 
            {
                LOG_ROOT_ERROR<<"检索数据"<<rsp.text<<" 结果反序列化失败";
                return Json::Value();
            }
            LOG_ROOT_DEBUG<<"检索成功";
            return json_res["hits"]["hits"];
        }
    private:
        std::unique_ptr<elasticlient::Client> _client;
    };
}
