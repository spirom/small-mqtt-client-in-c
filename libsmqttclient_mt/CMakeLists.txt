project(libsmqttclient_mt C)

add_definitions(-std=c99)

set(SOURCE_FILES
        smqttc_mt.c connection.c connection.h)

include_directories(
        ../inc
        ../libsmqttcore)

add_library(smqttclient_mt STATIC ${SOURCE_FILES})

target_link_libraries(smqttclient_mt smqttcore pthread)