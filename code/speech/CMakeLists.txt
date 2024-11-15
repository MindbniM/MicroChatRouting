CMAKE_MINIMUM_REQUIRED(VERSION 3.20)
project(Speech_Server)

set(CMAKE_CXX_STANDARD 17)

set(target "SpeechServer")
set(ProtoPath ${CMAKE_CURRENT_SOURCE_DIR}/../interface)
set(ProtoFile "speech.proto")

set(SpeechPath ${CMAKE_CURRENT_BINARY_DIR})
set(Speechpb_h ${SpeechPath}/speech.pb.h )
set(Speechpb_c ${SpeechPath}/speech.pb.cc)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../util)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

if(NOT EXISTS ${Speechpb_h} OR NOT EXISTS ${Speechpb_c})
    add_custom_command(
        OUTPUT ${Speechpb_c} ${Speechpb_h}
        COMMAND protoc
        ARGS --proto_path=${ProtoPath} --cpp_out=${CMAKE_CURRENT_BINARY_DIR}  ${ProtoFile}
        DEPENDS ${ProtoPath}/${ProtoFile}
        COMMENT "生成Protobuf框架代码文件:" ${Speechpb_h} ${Speechpb_c}
    )
endif()

set(Src ${CMAKE_CURRENT_SOURCE_DIR}/speech_server.cc ${Speechpb_c})

add_executable(${target} ${Src})
target_link_libraries(${target} -lgflags  -lbrpc -lssl -lcrypto -lprotobuf -lleveldb -letcd-cpp-api -lcpprest -lcurl -ljsoncpp -lpthread -ldl)
