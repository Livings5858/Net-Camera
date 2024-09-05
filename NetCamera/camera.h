#ifndef CAMERA_H
#define CAMERA_H

#include <string.h>
#include <linux/videodev2.h>

int initCamera();

int startStreaming();

int stopStreaming();

int takePicture(void** addr, size_t* length, struct v4l2_buffer *usedbuf);
int returnBuf(struct v4l2_buffer *usedbuf);

int deInitCamera();

#endif
