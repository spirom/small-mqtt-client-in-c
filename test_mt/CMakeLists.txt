project(smqtt_test_mt C)

add_definitions(-std=c99)

set(SOURCE_FILES
        main.c client.c)

include_directories(
        ../inc
        ../libsmqttcore
        ../libsmqttclient_mt
        ../libtest)

add_executable(smqtt_test_mt ${SOURCE_FILES})

target_link_libraries(smqtt_test_mt smqttcore smqttclient_mt test)