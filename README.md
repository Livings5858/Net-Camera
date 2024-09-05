# 网络摄像头

## 介绍

基于Linux V4L2框架捕获图像，结合MQTT发布。

* Linux: https://www.kernel.org/
* paho.mqtt.embedded-c: https://github.com/eclipse/paho.mqtt.embedded-c

## 目录结构

```
├── CMakeLists.txt
├── MQTTClient-C                    // MQTT基于平台的实现
│   ├── CMakeLists.txt
│   └── src
│       ├── CMakeLists.txt
...
│       └── linux                   // MQTT基于Linux平台的实现
│           ├── MQTTLinux.c
│           └── MQTTLinux.h
├── MQTTPacket                      // MQTT协议的实现
│   ├── CMakeLists.txt
│   └── src
│       ├── CMakeLists.txt
│       ├── MQTTConnect.h
│       ├── MQTTConnectClient.c
...
├── NetCamera                       // 网络摄像头
│   ├── CMakeLists.txt
│   ├── camera.c                    // 基于v4l2的图片捕获，使用了mmap方式映射内核buffer
│   ├── camera.h
│   └── main.c                      // 拍照并通过MQTT发布
```

## 快速开始

### 编译

> 需要先安装好CMake， 其他编译方法后续更新

```bash
mkdir build
cd build
cmake ..
make
```

### 执行

> 编译生成的文件在 `build/NetCamera/netcamera`

```bash
./netcamera
```

## 关联仓库

* 火灾检测：https://github.com/Livings5858/Fire-Detection
* MQTT 数据展示后端实现：https://github.com/Livings5858/test-server
* MQTT 数据展示前端实现：https://github.com/Livings5858/test-ui
