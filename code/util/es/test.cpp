#include <iostream>
#include <vector>
#include <string>
#include"icsearch.hpp"
using namespace std;
using namespace MindbniM;

int main() 
{
    LOG_ROOT_ADD_STDOUT_APPEND_DEFAULT();
    ESClient cl;
    ESClient::SearchValue sv("user");
    sv.append_should_match("phone.keyword","122");
    Json::Value ret=cl.Search(sv);
    int n=ret.size();
    for(int i=0;i<n;i++)
    {
        cout<<ret[i]["_source"]["name"]<<endl;
    }
    //ESClient::InsertValue iv("user");
    //iv.append("name","wang").append("phone","122").append("des","none");
    //cl.insert(iv,"0001");
    return 0;
}

