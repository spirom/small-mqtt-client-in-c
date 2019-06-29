project(libsmqttcore C)

add_definitions(-std=c99)

set(SOURCE_FILES
        local_server.c
        network_server.c
        server.c
        server.h
        messages.c
        messages.h
        messages_internal.h)

include_directories(../inc)

add_library(smqttcore STATIC ${SOURCE_FILES})