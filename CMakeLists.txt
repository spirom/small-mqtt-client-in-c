cmake_minimum_required(VERSION 3.3)
project(smqttc C)

ADD_SUBDIRECTORY(libsmqttcore)
ADD_SUBDIRECTORY(libsmqttclient_mt)
ADD_SUBDIRECTORY(libsmqttclient)
ADD_SUBDIRECTORY(smqttc)
ADD_SUBDIRECTORY(libtest)
ADD_SUBDIRECTORY(test)
ADD_SUBDIRECTORY(test_mt)





