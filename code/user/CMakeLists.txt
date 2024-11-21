CMAKE_MINIMUM_REQUIRED(VERSION 3.20)
project(User_Server)

set(CMAKE_CXX_STANDARD 17)


set(target "UserServer")

set(ProtoPath ${CMAKE_CURRENT_SOURCE_DIR}/../interface)
set(ProtoFile "user.proto")
set(basepro "base.proto")
set(Filepro "file.proto")

set(UserPath ${CMAKE_CURRENT_BINARY_DIR})
set(Userpb_h ${UserPath}/user.pb.h )
set(Userpb_c ${UserPath}/user.pb.cc)
set(basepb_h ${UserPath}/base.pb.h)
set(basepb_c ${UserPath}/base.pb.cc)
set(Filepb_h ${UserPath}/file.pb.h)
set(Filepb_c ${UserPath}/file.pb.c)

set(User_odb ${CMAKE_CURRENT_SOURCE_DIR}/../odb/user.hxx)
set(odbPath ${CMAKE_CURRENT_BINARY_DIR})
set(User_odb_h ${odbPath}/user-odb.hxx)
set(User_odb_c ${odbPath}/user-odb.cxx)
set(User_odb_i ${odbPath}/user-odb.ixx)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../util)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../odb)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../client)
include_directories(/usr/include/python3.8)

if(NOT EXISTS ${Userpb_h} OR NOT EXISTS ${Userpb_c} OR NOT EXISTS ${basepb_h} OR NOT EXISTS ${basepb_c} OR NOT EXISTS ${Filepb_h} OR NOT EXISTS ${Filepb_c})
    add_custom_command(
        OUTPUT ${Userpb_c} ${Userpb_h} ${basepb_h} ${basepb_c} ${Filepb_h} ${Filepb_c}
        COMMAND protoc
        ARGS --proto_path=${ProtoPath} --cpp_out=${CMAKE_CURRENT_BINARY_DIR}  ${ProtoFile} ${basepro} ${Filepro}
        DEPENDS ${ProtoPath}/${ProtoFile}
        COMMENT "生成Protobuf框架代码文件:" ${Userpb_h} ${Userpb_c}
    )
endif()

if(NOT EXISTS ${User_odb_h} OR NOT EXISTS ${User_odb_c} OR NOT EXISTS ${User_odb_i})
    add_custom_command(
        OUTPUT ${User_odb_h} ${User_odb_c} ${User_odb_i}
        COMMAND odb
        ARGS --database mysql --std c++17 --generate-query --generate-schema ${User_odb}
        DEPENDS ${User_odb}
    )
endif()

set(Src ${CMAKE_CURRENT_SOURCE_DIR}/user_server.cc ${Userpb_c} ${basepb_c} ${User_odb_c})

link_directories(/usr/local/lib)

add_executable(${target} ${Src})
target_link_libraries(${target} -lgflags -lbrpc /usr/lib/libssl.so /usr/lib/libcrypto.so -lprotobuf 
    -lleveldb -letcd-cpp-api -lcpprest -lcurl -ljsoncpp -lpthread -ldl -lodb-mysql -lodb -lodb-boost 
    -lpython3.8 -lhiredis -lredis++ -lelasticlient)

