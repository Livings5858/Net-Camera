add_executable(
  netcamera
  main.c
  camera.c
)
target_link_libraries(netcamera paho-embed-mqtt3cc paho-embed-mqtt3c)
target_include_directories(netcamera PRIVATE "../../src" "../../src/linux")
target_compile_definitions(netcamera PRIVATE MQTTCLIENT_PLATFORM_HEADER=MQTTLinux.h)
