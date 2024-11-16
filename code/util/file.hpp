#pragma once
#include "log.hpp"
namespace MindbniM
{
    class File
    {
    public:
        bool Read(const std::string &filename, std::string &body)
        {
            // 实现读取一个文件的所有数据，放入body中
            std::ifstream ifs(filename, std::ios::binary | std::ios::in);
            if (ifs.is_open() == false)
            {
                LOG_ROOT_ERROR<<"打开文件"<<filename<<"失败！";
                return false;
            }
            ifs.seekg(0, std::ios::end); // 跳转到文件末尾
            size_t flen = ifs.tellg();   // 获取当前偏移量-- 文件大小
            ifs.seekg(0, std::ios::beg); // 跳转到文件起始
            body.resize(flen);
            ifs.read(&body[0], flen);
            if (ifs.good() == false)
            {
                LOG_ROOT_ERROR<<"读取文件"<<filename<<"失败！";
                ifs.close();
                return false;
            }
            ifs.close();
            return true;
        }
        bool Write(const std::string &filename, const std::string &body)
        {
            // 实现将body中的数据，写入filename对应的文件中
            std::ofstream ofs(filename, std::ios::out | std::ios::binary | std::ios::trunc);
            if (ofs.is_open() == false)
            {
                LOG_ROOT_ERROR<<"打开文件"<<filename<<"失败！";
                return false;
            }
            ofs.write(body.c_str(), body.size());
            if (ofs.good() == false)
            {
                LOG_ROOT_ERROR<<"写入文件"<<filename<<"失败！";
                ofs.close();
                return false;
            }
            ofs.close();
            return true;
        }
    };
}