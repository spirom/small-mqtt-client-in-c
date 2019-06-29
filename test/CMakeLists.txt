project(smqtt_test C)

add_definitions(-std=c99)

set(SOURCE_FILES
        main.c encoding.c serialization.c client.c)

include_directories(
        ../inc
        ../libsmqttcore
        ../libsmqttclient
        ../libtest)

add_executable(smqtt_test ${SOURCE_FILES})

target_link_libraries(smqtt_test smqttcore smqttclient test)
