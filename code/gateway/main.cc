#include"gateway_server.hpp"
using namespace MindbniM;
DEFINE_string(log_file, "stdout", "用于指定日志的输出文件");
DEFINE_int32(log_level, 1, "用于指定日志输出等级");

DEFINE_int32(http_listen_port, 9000, "HTTP服务器监听端口");
DEFINE_int32(websocket_listen_port, 9001, "Websocket服务器监听端口");

DEFINE_string(registry_host, "http://127.0.0.1:2379", "服务注册中心地址");
DEFINE_string(base_service, "/service", "服务监控根目录");

DEFINE_string(redis_host, "127.0.0.1", "Redis服务器访问地址");
DEFINE_int32(redis_port, 6379, "Redis服务器访问端口");
DEFINE_int32(redis_db, 0, "Redis默认库号");
DEFINE_bool(redis_keep_alive, true, "Redis长连接保活选项");

int main(int argc, char *argv[])
{
    google::ParseCommandLineFlags(&argc, &argv, true);
    LoggerManager::GetInstance()->InitRootLog(FLAGS_log_file,FLAGS_log_level);
    GatewayServerBuild gsb;
    gsb.make_redis_object(FLAGS_redis_host, FLAGS_redis_port, FLAGS_redis_db);
    gsb.make_dis_object(FLAGS_registry_host, FLAGS_base_service);
    auto server = gsb.newGatewayServer(FLAGS_websocket_listen_port, FLAGS_http_listen_port);
    server->start();
    return 0;
}