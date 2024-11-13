CMAKE_MINIMUM_REQUIRED(VERSION 3.20)
project(Speech_Server)

set(SpeechPath ${CMAKE_CURRENT_SOURCE_DIR})
set(Speechpbs SpeechPath/speech.pb.h Speech)

set(Speechpb_h "")
set(Speechpb_cc "")
foreach(speechpb speechpbs)
    if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}${Speechpb_h})