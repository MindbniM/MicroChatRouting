CMAKE_MINIMUM_REQUIRED(VERSION 3.20)
project(Message_Server)

set(CMAKE_CXX_STANDARD 17)


set(target "MessageServer")

set(ProtoPath ${CMAKE_CURRENT_SOURCE_DIR}/../interface)
set(MessagePro "message.proto")
set(BasePro "base.proto")
set(UserPro "user.proto")
set(FilePro "file.proto")

set(MessagePath ${CMAKE_CURRENT_BINARY_DIR})
set(Messagepb_h ${MessagePath}/message.pb.h )
set(Messagepb_c ${MessagePath}/message.pb.cc)
set(Basepb_h ${MessagePath}/base.pb.h)
set(Basepb_c ${MessagePath}/base.pb.cc)
set(Userpb_h ${MessagePath}/user.pb.h)
set(Userpb_c ${MessagePath}/user.pb.cc)
set(Filepb_h ${MessagePath}/file.pb.h)
set(Filepb_c ${MessagePath}/file.pb.cc)

set(Message_odb ${CMAKE_CURRENT_SOURCE_DIR}/../odb/message.hxx)
set(odbPath ${CMAKE_CURRENT_BINARY_DIR})
set(Message_odb_h ${odbPath}/message-odb.hxx)
set(Message_odb_c ${odbPath}/message-odb.cxx)
set(Message_odb_i ${odbPath}/message-odb.ixx)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../util)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../odb)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../client)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../mysql)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../es)

function(CreateProto Pro Propb_h Propb_c)
    if(NOT EXISTS ${Propb_h} OR NOT EXISTS ${Propb_c})
        add_custom_command(
            OUTPUT ${Propb_h} ${Propb_c}
            COMMAND protoc
            ARGS --proto_path=${ProtoPath} --cpp_out=${CMAKE_CURRENT_BINARY_DIR}  ${Pro}
            DEPENDS ${ProtoPath}/${Pro}
            COMMENT "生成Protobuf框架代码文件:" ${Propb_h} ${Propb_c}
        )
    endif()
endfunction()
function(CreateOdb Odb Odb_h Odb_c Odb_i)
    if(NOT EXISTS ${Odb_h} OR NOT EXISTS ${Odb_c} OR NOT EXISTS ${Odb_i})
        add_custom_command(
            OUTPUT ${Odb_h} ${Odb_c} ${Odb_i}
            COMMAND odb
            ARGS --database mysql --std c++17 --generate-query --generate-schema --profile boost/date-time ${Odb}
            DEPENDS ${Odb}
            COMMENT "生成ODB框架代码文件:" ${Odb_h} ${Odb_c} ${Odb_i}
        )
    endif()
endfunction()

CreateProto(${MessagePro} ${Messagepb_h} ${Messagepb_c})
CreateProto(${BasePro} ${Basepb_h} ${Basepb_c})
CreateProto(${UserPro} ${Userpb_h} ${Userpb_c})
CreateProto(${FilePro} ${Filepb_h} ${Filepb_c})

CreateOdb(${Message_odb} ${Message_odb_h} ${Message_odb_c} ${Message_odb_i})

set(Src ${CMAKE_CURRENT_SOURCE_DIR}/message_server.cc ${Messagepb_c} ${Basepb_c} ${Message_odb_c} ${Userpb_c} ${Filepb_c}) 

link_directories(/usr/local/lib)

add_executable(${target} ${Src})
target_link_libraries(${target}  -lbrpc /usr/lib/libssl.so /usr/lib/libcrypto.so -lprotobuf -lelasticlient
    -lleveldb -letcd-cpp-api -lcpprest -lcurl -ljsoncpp -lpthread -ldl -lodb-mysql -lodb -lodb-boost -lamqpcpp -lev -lgflags)

