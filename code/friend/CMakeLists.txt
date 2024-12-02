CMAKE_MINIMUM_REQUIRED(VERSION 3.20)
project(Friend_Server)

set(CMAKE_CXX_STANDARD 17)

set(target "FriendServer")

set(ProtoPath ${CMAKE_CURRENT_SOURCE_DIR}/../interface)
set(PROFILES "base.proto" "friend.proto" "user.proto" "message.proto")

set(OdbPath ${CMAKE_CURRENT_SOURCE_DIR}/../odb)
set(ODBFIES "friend_apply.hxx" "chat_session.hxx" "relation.hxx" "chat_session_member.hxx")


set(OutPath ${CMAKE_CURRENT_BINARY_DIR})

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../util)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../odb)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../client)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../mysql)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../es)

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

set(ODBCC "")
function(CreateOdb OdbPath OdbFile OutPath)
    get_filename_component(BASENAME ${OdbFile} NAME_WE)
    set(Odb_h ${OutPath}/${BASENAME}-odb.hxx)
    set(Odb_c ${OutPath}/${BASENAME}-odb.cxx)
    set(Odb_i ${OutPath}/${BASENAME}-odb.ixx)
    set(Odb ${OdbPath}/${OdbFile})
    
    if(NOT EXISTS ${Odb_h} OR NOT EXISTS ${Odb_c} OR NOT EXISTS ${Odb_i})
        add_custom_command(
            COMMAND echo "Generating Odb files for ${OdbFile}"  # 调试信息
            OUTPUT ${Odb_h} ${Odb_c} ${Odb_i}
            COMMAND odb
            ARGS --database mysql --std c++17 --generate-query --generate-schema --profile boost/date-time --output-dir ${OutPath} ${Odb}
            DEPENDS ${Odb}
            COMMENT "生成ODB框架代码文件:" ${Odb_h} ${Odb_c} ${Odb_i}
        )
    endif()

    list(APPEND ODBCC ${Odb_c})
    set(ODBCC ${ODBCC} PARENT_SCOPE)
endfunction()

foreach(PROFILE ${PROFILES})
    CreateProto(${ProtoPath} ${PROFILE} ${OutPath})
endforeach()

foreach(ODBFILE ${ODBFIES})
    CreateOdb(${OdbPath} ${ODBFILE} ${OutPath})
endforeach()

set(Src ${PROCC} ${ODBCC}) 
file(GLOB _Src ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)
list(APPEND Src ${_Src})

link_directories(/usr/local/lib)

add_executable(${target} ${Src})
target_link_libraries(${target}  -lbrpc /usr/lib/libssl.so /usr/lib/libcrypto.so -lprotobuf -lelasticlient
    -lleveldb -letcd-cpp-api -lcpprest -lcurl -ljsoncpp -lpthread -ldl -lodb-mysql -lodb -lodb-boost -lamqpcpp -lev -lgflags)

