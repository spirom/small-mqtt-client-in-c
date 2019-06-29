project(smqttc C)

add_definitions(-std=c99)

set(SOURCE_FILES
        main.c)

include_directories(../inc)

add_executable(smqttc ${SOURCE_FILES})

target_link_libraries(smqttc smqttclient)