#include<gflags/gflags.h>
#include"speech_server.hpp"
using namespace MindbniM;
DEFINE_string(log_append,"stdout","日志输出地");
DEFINE_int32(log_level,1,"最低日志等级");

DEFINE_string(registry_host, "http://127.0.0.1:2379", "服务注册中心地址");
DEFINE_string(base_service, "/service", "服务监控根目录");
DEFINE_string(instance_name, "/speech_service/instance", "当前实例名称");
DEFINE_string(access_host, "127.0.0.1:10001", "当前实例的外部访问地址");

DEFINE_int32(listen_port, 10001, "Rpc服务器监听端口");
DEFINE_int32(rpc_timeout, -1, "Rpc调用超时时间");
DEFINE_int32(rpc_threads, 1, "Rpc的IO线程数量");


int main(int argc,char* argv[])
{
    LoggerManager::GetInstance()->InitRootLog(FLAGS_log_append,FLAGS_log_level);
    google::ParseCommandLineFlags(&argc,&argv,true);
    SpeechServerBuild sb;
    sb.make_reg_object(FLAGS_registry_host,FLAGS_base_service+FLAGS_instance_name,FLAGS_access_host);
    sb.make_rpc_server(FLAGS_listen_port,FLAGS_rpc_timeout,FLAGS_rpc_threads);
    SpeechServer::ptr speech_server=sb.newSpeechServer();
    speech_server->start();
    return 0;
}
