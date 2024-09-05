#include <stdio.h>
#include <memory.h>
#include "MQTTClient.h"

#include <stdio.h>
#include <signal.h>

#include <sys/time.h>

#include "camera.h"

volatile int toStop = 0;

#define CHUNK_SIZE 64

void cfinish(int sig)
{
    signal(SIGINT, NULL);
    toStop = 1;
}


struct opts_struct
{
    char* clientid;
    int nodelimiter;
    char* delimiter;
    enum QoS qos;
    char* username;
    char* password;
    char* host;
    int port;
    int showtopics;
} opts =
{
    (char*)"stdout-subscriber", 0, (char*)"\n", QOS2, NULL, NULL, (char*)"192.168.3.100", 1884, 0
};

void messageArrived(MessageData* md)
{
    MQTTMessage* message = md->message;

    if (opts.showtopics)
        printf("%.*s\t", md->topicName->lenstring.len, md->topicName->lenstring.data);
    if (opts.nodelimiter)
        printf("%.*s", (int)message->payloadlen, (char*)message->payload);
    else
        printf("%.*s%s", (int)message->payloadlen, (char*)message->payload, opts.delimiter);
    //fflush(stdout);
}

int sendImage(MQTTClient *client, const char* topic, void* image_addr, size_t size) {

    unsigned char buffer[CHUNK_SIZE];
    int chunk_id = 0;
    size_t readySize = 0;
    size_t offset = 0;

    while (offset < size) {
        int chunk_size = (size - offset > CHUNK_SIZE) ? CHUNK_SIZE : size - offset;

        // 构建消息，包含分段ID和数据
        char payload[CHUNK_SIZE + 10]; // 留出ID等的空间
        sprintf(payload, "%04d:", chunk_id); // 添加分段ID
        memcpy(payload + 5, image_addr + offset, chunk_size);      // 拷贝图像数据

        // 创建MQTT消息
        MQTTMessage pubmsg;

        memset(&pubmsg, 0, sizeof(pubmsg));
        pubmsg.payload = payload;
        pubmsg.payloadlen = (int)chunk_size + 5;
        pubmsg.qos = QOS0;
        pubmsg.dup = 0;
        pubmsg.retained = 0;

        // 发送消息
        int ret = MQTTPublish(client, topic, &pubmsg);
        chunk_id++; // 增加分段ID
        offset += chunk_size;
        if (chunk_id % 50 == 0 || size <= offset) {
            printf("Publishing to %s, chunk_id:%d, chunk_size:%u, ret:%d\n",
                topic, chunk_id, chunk_size, ret);
        }
    }
    char donemsg[] = "done";
    MQTTMessage msg = {
        .qos = QOS0,
        .retained = 0,
        .payload = donemsg,
        .payloadlen = strlen(donemsg),
        .id = 0,
        .dup = 0
    };
    MQTTPublish(client, "image_send_done", &msg);
    return 0;
}


int main(int argc, char** argv)
{
    int rc = 0;
    unsigned char buf[100];
    unsigned char readbuf[100];
    
    
    char* topic = "control";

    if (strchr(topic, '#') || strchr(topic, '+'))
        opts.showtopics = 1;
    if (opts.showtopics)
        printf("topic is %s\n", topic);

    Network n;
    MQTTClient c;

    signal(SIGINT, cfinish);
    signal(SIGTERM, cfinish);

    NetworkInit(&n);
    NetworkConnect(&n, opts.host, opts.port);
    MQTTClientInit(&c, &n, 1000, buf, 100, readbuf, 100);
 
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;       
    data.willFlag = 0;
    data.MQTTVersion = 3;
    data.clientID.cstring = opts.clientid;
    data.username.cstring = opts.username;
    data.password.cstring = opts.password;

    data.keepAliveInterval = 10;
    data.cleansession = 1;
    printf("Connecting to %s %d\n", opts.host, opts.port);
    
    rc = MQTTConnect(&c, &data);
    printf("Connected %d\n", rc);
    
    printf("Subscribing to %s\n", topic);
    rc = MQTTSubscribe(&c, topic, opts.qos, messageArrived);
    printf("Subscribed %d\n", rc);

    void* imageData = NULL;
    size_t imageDataSize = 0;
    struct v4l2_buffer usedbuf;
    initCamera();
    startStreaming();
    while (!toStop)
    {
        takePicture(&imageData, &imageDataSize, &usedbuf);
        sendImage(&c, "image_data", imageData, imageDataSize);
        returnBuf(&usedbuf);
        // 初始化随机数生成器
        srand(time(NULL));
        // 生成随机坐标
        int x = rand() % (100 + 1);
        int y = rand() % (100 + 1);
        int temperature = 10 + rand() % (40 - 10 + 1);
        char data[32] = {0};
        sprintf(data, "%d, %d, %d", temperature, x, y);
        MQTTMessage msg = {
            .qos = QOS0,
            .retained = 0,
            .payload = data,
            .payloadlen = strlen(data),
            .id = 0,
            .dup = 0
        };
        sleep(1);
        MQTTPublish(&c, "sensor", &msg);
        MQTTYield(&c, 1000);	
    }
    stopStreaming();
    deInitCamera();
    
    printf("Stopping\n");

    MQTTDisconnect(&c);
    NetworkDisconnect(&n);

    return 0;
}


