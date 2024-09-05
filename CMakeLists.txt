cmake_minimum_required(VERSION 2.8)
project (network-camera)
set(CMAKE_BUILD_TYPE Debug)

message(STATUS "CMake version: " ${CMAKE_VERSION})
message(STATUS "CMake system name: " ${CMAKE_SYSTEM_NAME})

set(CMAKE_SCRIPTS "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")

include_directories(MQTTPacket/src)
include_directories(MQTTClient-C/src)
include_directories(MQTTClient-C/src/linux)

ADD_SUBDIRECTORY(MQTTPacket)
ADD_SUBDIRECTORY(MQTTClient-C)
ADD_SUBDIRECTORY(NetCamera)
