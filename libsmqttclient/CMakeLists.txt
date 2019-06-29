project(libsmqttclient C)

add_definitions(-std=c99)

set(SOURCE_FILES
        smqttc.c
        smqttc_internal.h)

include_directories(
        ../inc
        ../libsmqttcore)

add_library(smqttclient STATIC ${SOURCE_FILES})

target_link_libraries(smqttclient smqttcore)

