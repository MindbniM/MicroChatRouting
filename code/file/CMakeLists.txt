CMAKE_MINIMUM_REQUIRED(VERSION 3.20)
project(File_Server)

set(CMAKE_CXX_STANDARD 17)


set(target "FileServer")
#
set(test "test")

set(ProtoPath ${CMAKE_CURRENT_SOURCE_DIR}/../interface)
set(ProtoFile "file.proto")
set(basepro "base.proto")

set(FilePath ${CMAKE_CURRENT_BINARY_DIR})
set(Filepb_h ${FilePath}/file.pb.h )
set(Filepb_c ${FilePath}/file.pb.cc)
set(basepb_h ${FilePath}/base.pb.h)
set(basepb_c ${FilePath}/base.pb.cc)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../util)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

if(NOT EXISTS ${Filepb_h} OR NOT EXISTS ${Filepb_c} OR NOT EXISTS ${basepb_h} OR NOT EXISTS ${basepb_c})
    add_custom_command(
        OUTPUT ${Filepb_c} ${Filepb_h} ${basepb_h} ${basepb_c}
        COMMAND protoc
        ARGS --proto_path=${ProtoPath} --cpp_out=${CMAKE_CURRENT_BINARY_DIR}  ${ProtoFile} ${basepro}
        DEPENDS ${ProtoPath}/${ProtoFile}
        COMMENT "生成Protobuf框架代码文件:" ${Filepb_h} ${Filepb_c}
    )
endif()

set(Src ${CMAKE_CURRENT_SOURCE_DIR}/file_server.cc ${Filepb_c} ${basepb_c})

link_directories(/usr/local/lib)

add_executable(${target} ${Src})
target_link_libraries(${target} -lgflags -lbrpc /usr/lib/libssl.so /usr/lib/libcrypto.so -lprotobuf -lleveldb -letcd-cpp-api -lcpprest -lcurl -ljsoncpp -lpthread -ldl)
#
add_executable(${test} ${CMAKE_CURRENT_SOURCE_DIR}/test.cc ${Filepb_c} ${basepb_c})
target_link_libraries(${test} -lgflags -lbrpc /usr/lib/libssl.so /usr/lib/libcrypto.so -lprotobuf -lleveldb -letcd-cpp-api -lcpprest -lcurl -ljsoncpp -lpthread -ldl -lgtest)

