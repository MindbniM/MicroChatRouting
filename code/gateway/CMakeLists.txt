CMAKE_MINIMUM_REQUIRED(VERSION 3.20)
project(User_Server)

set(CMAKE_CXX_STANDARD 17)

set(target "GatwayServer")

set(ProtoPath ${CMAKE_CURRENT_SOURCE_DIR}/../interface)
set(PROFILES "base.proto" "user.proto" "file.proto" "friend.proto" "message.proto" "speech.proto" "transmite.proto" "getway.proto")


set(OutPath ${CMAKE_CURRENT_SOURCE_DIR}/../temp)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../util)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../client)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../temp)

set(PROCC "")
function(CreateProto ProPath ProFile OutPath)
    get_filename_component(BASENAME ${ProFile} NAME_WE)
    set(Propb_h ${OutPath}/${BASENAME}.pb.h)
    set(Propb_c ${OutPath}/${BASENAME}.pb.cc)
    set(Pro ${ProPath}/${ProFile})
    if(NOT EXISTS ${Propb_h} OR NOT EXISTS ${Propb_c})
        add_custom_command(
            OUTPUT ${Propb_h} ${Propb_c}
            COMMAND protoc
            ARGS --proto_path=${ProPath} --cpp_out=${OutPath}  ${ProFile}
            DEPENDS ${Pro}
            COMMENT "生成Protobuf框架代码文件:" ${Propb_h} ${Propb_c}
        )
    endif()
    list(APPEND PROCC ${Propb_c})
    set(PROCC ${PROCC} PARENT_SCOPE)
endfunction()


foreach(PROFILE ${PROFILES})
    CreateProto(${ProtoPath} ${PROFILE} ${OutPath})
endforeach()

set(Src ${PROCC}) 
file(GLOB _Src ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)
list(APPEND Src ${_Src})

link_directories(/usr/local/lib)

add_executable(${target} ${Src})
#