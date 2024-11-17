#pragma once
#include<iomanip>
#include<random>
#include<atomic>
namespace MindbniM
{
    class UUID
    {
    public:
        std::string static Get()
        {
            std::random_device rd;
            std::mt19937 key(rd());
            std::uniform_int_distribution<int> dis(0,255);
            std::stringstream ss;
            for(int i=0;i<6;i++)
            {
                if(i==2)ss<<"-";
                ss<<std::setw(2)<<std::setfill('0')<<std::hex<<dis(key);
            }
            ss<<"-";
            static std::atomic<int> num(0);
            short temp=num++;
            ss<<std::setw(4)<<std::setfill('0')<<std::hex<<temp;
            return ss.str();
        }
    };
}