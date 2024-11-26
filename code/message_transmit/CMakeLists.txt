CMAKE_MINIMUM_REQUIRED(VERSION 3.20)
project(Transmite_Server)

set(CMAKE_CXX_STANDARD 17)


set(target "TransmiteServer")

set(ProtoPath ${CMAKE_CURRENT_SOURCE_DIR}/../interface)
set(TransmitePro "transmite.proto")
set(BasePro "base.proto")
set(UserPro "user.proto")

set(TransmitePath ${CMAKE_CURRENT_BINARY_DIR})
set(Transmitepb_h ${TransmitePath}/transmite.pb.h )
set(Transmitepb_c ${TransmitePath}/transmite.pb.cc)
set(Basepb_h ${TransmitePath}/base.pb.h)
set(Basepb_c ${TransmitePath}/base.pb.cc)
set(Userpb_h ${TransmitePath}/user.pb.h)
set(Userpb_c ${TransmitePath}/user.pb.cc)

set(Transmite_odb ${CMAKE_CURRENT_SOURCE_DIR}/../odb/chat_session_member.hxx)
set(odbPath ${CMAKE_CURRENT_BINARY_DIR})
set(Transmite_odb_h ${odbPath}/chat_session_member-odb.hxx)
set(Transmite_odb_c ${odbPath}/chat_session_member-odb.cxx)
set(Transmite_odb_i ${odbPath}/chat_session_member-odb.ixx)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../util)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../odb)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../client)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../mysql)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../es)

if(NOT EXISTS ${Transmitepb_h} OR NOT EXISTS ${Transmitepb_c} OR NOT EXISTS ${Basepb_h} OR NOT EXISTS ${Basepb_c} OR NOT EXISTS ${Userpb_h} OR NOT EXISTS ${Userpb_c})
    add_custom_command(
        OUTPUT ${Transmitepb_c} ${Transmitepb_h} ${Basepb_h} ${Basepb_c} ${Userpb_h} ${Userpb_c}
        COMMAND protoc
        ARGS --proto_path=${ProtoPath} --cpp_out=${CMAKE_CURRENT_BINARY_DIR}  ${TransmitePro} ${BasePro} ${UserPro}
        DEPENDS ${ProtoPath}/${TransmitePro}
        COMMENT "生成Protobuf框架代码文件:" ${Transmitepb_h} ${Transmitepb_c}
    )
endif()

if(NOT EXISTS ${Transmite_odb_h} OR NOT EXISTS ${Transmite_odb_c} OR NOT EXISTS ${Transmite_odb_i})
    add_custom_command(
        OUTPUT ${Transmite_odb_h} ${Transmite_odb_c} ${Transmite_odb_i}
        COMMAND odb
        ARGS --database mysql --std c++17 --generate-query --generate-schema ${Transmite_odb}
        DEPENDS ${Transmite_odb}
        COMMENT "生成ODB框架代码文件:" ${Transmite_odb_h} ${Transmite_odb_c} ${Transmite_odb_i}
    )
endif()

set(Src ${CMAKE_CURRENT_SOURCE_DIR}/transmite_server.cc ${Transmitepb_c} ${Basepb_c} ${Transmite_odb_c} ${Userpb_c})

link_directories(/usr/local/lib)

add_executable(${target} ${Src})
target_link_libraries(${target}  -lbrpc /usr/lib/libssl.so /usr/lib/libcrypto.so -lprotobuf 
    -lleveldb -letcd-cpp-api -lcpprest -lcurl -ljsoncpp -lpthread -ldl -lodb-mysql -lodb -lodb-boost -lamqpcpp -lev -lgflags)

