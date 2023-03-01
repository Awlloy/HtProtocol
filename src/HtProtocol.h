                                                        
#ifndef HTPROTOCOL_H
#define HTPROTOCOL_H

#include <malloc.h>
#include <stdio.h>

typedef struct HtContext HtContext;
typedef struct PrivData PrivData;

//最大支持通信连接数量,请根据需求修改，会影响到内存占用
#define MAX_CONNECT 32

//帧头尾标志，最高位必须位1
#define HEADFLAG 0xab
//数据包内出现FLAG的话进行字节填充填充（除了头部数据其他数据皆填充），最高位必须位1
#define PADFLAG 0xde
#define NUMBER_MAX_SIZE 256
#define HTABS(x) ((x)>0)?(x):-(x)
enum HPFlag{
    ACK=0x40,//应答
    NAK=0x20,//请求
    LF=0x10,//最后一段
    RF=0x08,//最后一包接收者反馈
};

//窗口大小，窗口越大占用空间越大
#define WINDOW_SIZE 10
// #define READ_WINDOW_SIZE 30
#define READ_WINDOW_SIZE 10
#define PACK_SIZE 20


typedef struct WindowFifo WindowFifo;
typedef struct HtProtocolContext HtProtocolContext;
typedef struct HtBuffer HtBuffer;
typedef struct HTimestamp HTimestamp;
typedef struct HtProtocolStruct HtProtocolStruct;
typedef struct HtContext HtContext;
typedef struct PrivData PrivData;
typedef int (*HtReadwriteFunc)(void *buf,int size,int time_out,PrivData *priv);//成功返回0
#define CONTEXT_SIZE sizeof(HtProtocolContext)

struct PrivData{
    void *priv;
    int size;
};
struct HtBuffer{
    unsigned char buf[PACK_SIZE];
    unsigned short size;
    // int number;
    unsigned char number;

};

/**
 * 包头
 * head 8位
 * flag 8位标志
 * num  8位序号 
 * data n*8位
 * PADFLAG 8位
 * ver 8位校验
 * 
 * 分包
 * flag 8位
 * num  8位序号
 * data n*8位
 * PADFLAG 8位
 * ver 8位校验
 * 
 * 尾包
 * flag 8位标志
 * num  8位序号
 * data n*8位
 * PADFLAG 8位
 * ver 8位校验
 * 
 * 序号解释：
 * 如果一帧能发20个字节，则最大发送 256*(20-4） - 1(帧头尾) = 69630字节的数据
 * 
 * flag解释
 * 必须为0
 * 0    Ack    Nak          LF
 *  0  确认帧  错误帧（请求帧） 最后帧
 * */
struct WindowFifo{
    HtBuffer fifo[READ_WINDOW_SIZE];//接收窗口
    unsigned short head;//head为第一个数据
    unsigned short size;//fifo占用长度
};
struct HTimestamp{
    long long time_tamp;
};
struct HtProtocolContext{
    unsigned short pack_idx;
    //滑动窗口
    // unsigned short read_idx_head;
    // unsigned short read_idx_rear;
    WindowFifo write_fifo;
    unsigned char write_check[WINDOW_SIZE];
    HTimestamp write_timestamp[WINDOW_SIZE];
    long long retry_timeout_us;

    WindowFifo read_fifo;
    unsigned char read_check[READ_WINDOW_SIZE];
    char activate;
    
    HtReadwriteFunc read;//成功返回0
    HtReadwriteFunc write;//成功返回0
    PrivData priv;
};

struct HtContext{
    char pad[sizeof(HtProtocolContext)];
};
void timer_clock(long long pass_time_us);//时钟更新

// int init_protocol_context(HtProtocolContext *context,long long retry_timeout_us);
// void set_priv_data(HtProtocolContext *context,void *priv_data,int priv_size);
// int sendMessage(void *buf,int size,HtProtocolContext *context,int time_out);
// int readMessage(void *buf,int size,HtProtocolContext *context,int time_out);
int get_context_size();
HtContext *init_protocol_context(void *context,long long retry_timeout_us,HtReadwriteFunc write_func,HtReadwriteFunc read_func);
void set_priv_data(void *context,void *priv_data,int priv_size);
// void set_write_func(void *context,HtReadwriteFunc func);
// void set_read_func(void *context,HtReadwriteFunc func);
int sendMessage(void *buf,int size,void *context,int time_out);
int readMessage(void *buf,int size,void *context,int time_out);
#endif

