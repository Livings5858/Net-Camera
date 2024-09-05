#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <linux/videodev2.h>
#include <linux/v4l2-subdev.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <fcntl.h>

#include "camera.h"

#define BUF_NUM (4)

int gFd = 0;

struct buffer {
    void* addr;
    size_t length;
} gBuffers[BUF_NUM];


int initCamera()
{
    gFd = open("/dev/video1", O_RDWR | O_CLOEXEC);
    if (!gFd) {
        printf("open camera failed.\n");
        return -1;
    }
    printf("open camera ok, gFd = %d\n", gFd);
    return 0;
}

int setFormat()
{
    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    fmt.fmt.pix.width = 640;
    fmt.fmt.pix.height = 480;
    if (ioctl(gFd, VIDIOC_S_FMT, &fmt) < 0) {
        printf("VIDIOC_S_FMT failed - [%d]!\n", errno);
        return -1;
    }
    printf("VIDIOC_S_FMT OK!\n");
    printf("width %d, height %d, size %d, bytesperline %d, format %c%c%c%c\n\n",
           fmt.fmt.pix.width, fmt.fmt.pix.height, fmt.fmt.pix.sizeimage,
           fmt.fmt.pix.bytesperline,
           fmt.fmt.pix.pixelformat & 0xFF,
           (fmt.fmt.pix.pixelformat >> 8) & 0xFF,
           (fmt.fmt.pix.pixelformat >> 16) & 0xFF,
           (fmt.fmt.pix.pixelformat >> 24) & 0xFF);
    return 0;
}

int reqBuf()
{
    struct v4l2_requestbuffers req;
    memset(&req, 0, sizeof(req));
    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    if (ioctl(gFd, VIDIOC_REQBUFS, &req) == -1) {
        printf("VIDIOC_REQBUFS failed!\n\n");
        return -1;
    }
    if (4 != req.count) {
        printf("!!! req count = %d\n", req.count);
        return -1;
    }
    printf("VIDIOC_REQBUFS succeed!\n\n");
    return 0;
}

int queryBuf()
{
    for (int i = 0; i < BUF_NUM; i++) {
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.index = i;
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        if (ioctl(gFd, VIDIOC_QUERYBUF, &buf) == -1) {
            printf("VIDIOC_QUERYBUF failed!\n");
            return -1;
        }
        gBuffers[i].length = buf.length;
        gBuffers[i].addr = mmap(NULL,
                                buf.length,
                                PROT_READ | PROT_WRITE,
                                MAP_SHARED,
                                gFd,
                                buf.m.offset);
        printf("VIDIOC_QUERYBUF gBuffers[%d].addr=%p\n", i, gBuffers[i].addr);
    }
    return 0;
}

int queueAllBufs()
{
    for (int i = 0; i < BUF_NUM; i++) {
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.index = i;
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        if (ioctl(gFd, VIDIOC_QBUF, &buf) == -1) {
            printf("VIDIOC_QBUF failed!\n");
            return -1;
        }
        printf("VIDIOC_QBUF buf[%d] success\n", i);
    }
    return 0;
}

int streamOn()
{
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(gFd, VIDIOC_STREAMON, &type) == -1) {
        printf("VIDIOC_STREAMON failed!\n");
        return -1;
    }
    printf("VIDIOC_STREAMON success!\n");
    return 0;
}

int streamOff()
{
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(gFd, VIDIOC_STREAMOFF, &type) == -1) {
        printf("VIDIOC_STREAMOFF failed!\n");
        return -1;
    }
    printf("VIDIOC_STREAMOFF success!\n");
    return 0;
}

int startStreaming()
{
    setFormat();
    reqBuf();
    queryBuf();
    queueAllBufs();
    streamOn();
    sleep(3);
    return 0;
}

int stopStreaming()
{
    streamOff();
    return 0;
}

int savePicture(const char* path, void* image_data, size_t length)
{
    FILE* fp = fopen(path, "wb");
    if (!fp) {
        printf("open %s error\n", path);
        return -1;
    }
    if (fwrite(image_data, 1, length, fp) < length) {
        printf("Out of memory!\n");
        return -1;
    }
    fflush(fp);
    fclose(fp);
    return 0;
}

int takePicture(void** addr, size_t* length, struct v4l2_buffer *usedbuf)
{
    struct v4l2_buffer buf;
    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    if (ioctl(gFd, VIDIOC_DQBUF, &buf) == -1) {
        printf("VIDIOC_DQBUF failed!\n");
        return -1;
    }
    *addr = gBuffers[buf.index].addr;
    *length = buf.length;

    printf("takePicture addr=%p, length=%d\n", *addr, *length);

    *usedbuf = buf;
    return 0;
}

int returnBuf(struct v4l2_buffer *buf)
{
    if (ioctl(gFd, VIDIOC_QBUF, buf) == -1) {
        printf("VIDIOC_QBUF failed!\n");
        return -1;
    }
    return 0;
}

int deInitCamera()
{
    if (gFd) {
        close(gFd);
        printf("close camera ok, gFd = %d\n", gFd);
        gFd = 0;
    }
    return 0;
}

void get_capabilities()
{
    struct v4l2_capability cap;
    if (ioctl(gFd, VIDIOC_QUERYCAP, &cap) < 0) {
        printf("VIDIOC_QUERYCAP failed\n");
        return;
    }
    printf("------- VIDIOC_QUERYCAP ----\n");
    printf("  driver: %s\n", cap.driver);
    printf("  card: %s\n", cap.card);
    printf("  bus_info: %s\n", cap.bus_info);
    printf("  version: %d.%d.%d\n",
           (cap.version >> 16) & 0xff,
           (cap.version >> 8) & 0xff,
           (cap.version & 0xff));
    printf("  capabilities: %08X\n", cap.capabilities);

    if (cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)
        printf("        Video Capture\n");
    if (cap.capabilities & V4L2_CAP_VIDEO_OUTPUT)
        printf("        Video Output\n");
    if (cap.capabilities & V4L2_CAP_VIDEO_OVERLAY)
        printf("        Video Overly\n");
    if (cap.capabilities & V4L2_CAP_VIDEO_CAPTURE_MPLANE)
        printf("        Video Capture Mplane\n");
    if (cap.capabilities & V4L2_CAP_VIDEO_OUTPUT_MPLANE)
        printf("        Video Output Mplane\n");
    if (cap.capabilities & V4L2_CAP_READWRITE)
        printf("        Read / Write\n");
    if (cap.capabilities & V4L2_CAP_STREAMING)
        printf("        Streaming\n");
    printf("\n");
    return;
}