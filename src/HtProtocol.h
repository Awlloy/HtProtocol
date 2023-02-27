
#ifndef HTPROTOCOL_H
#define HTPROTOCOL_H
#include "val_data.h"
#include <malloc.h>
#include <stdio.h>


// #define PACK_SIZE 128
#define PACK_SIZE 20
//窗口大小，窗口越大占用空间越大
#define WINDOW_SIZE 10
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

typedef struct WindowFifo WindowFifo;
typedef struct HtProtocolContext HtProtocolContext;
typedef struct HtBuffer HtBuffer;
typedef struct HTimestamp HTimestamp;

// extern int64_th time_cout_us;
// extern HtProtocolContext *context_array[MAX_CONNECT];//保存连接的context,以便遍历检查
// extern int context_num;

struct HTimestamp{
    int64_th time_tamp;
};
struct HtBuffer{
    uint8_th buf[PACK_SIZE];
    uint16_th size;
    int number;

};

/**
 * 包头
 * head 8位
 * flag 8位标志
 * num  8位序号 
 * data n*8位
 * ver 8位校验
 * 
 * 分包
 * num  8位序号
 * data n*8位
 * ver 8位校验
 * 
 * 尾包
 * rear 8位
 * flag 8位标志
 * 
 * num  8位序号
 * data n*8位
 * ver 8位校验
 * 
 * 序号解释：
 * 如果一帧能发20个字节，则最大发送 256*(20-3） - 2(帧头尾) = 69630字节的数据
 * 
 * flag解释
 * 必须为0
 * 0    Ack    Nak          LF
 *  0  确认帧  错误帧（请求帧） 最后帧
 * */
struct WindowFifo{
    HtBuffer fifo[WINDOW_SIZE];//接收窗口
    uint16_th head;//head为第一个数据
    uint16_th rear;//rear不村数据
};
struct HtProtocolContext{
    uint16_th pack_idx;
    //滑动窗口
    // uint16_th read_idx_head;
    // uint16_th read_idx_rear;
    WindowFifo read_fifo;
    WindowFifo write_fifo;

    int64_th retry_timeout_us;
    HTimestamp write_timestamp[WINDOW_SIZE];
    uint8_th write_check[WINDOW_SIZE];
    uint8_th read_check[WINDOW_SIZE];
    int cp_pack_number;
    int8_th activate;
    int (*read)(void *buf,int size,int time_out);//成功返回0
    int (*write)(void *buf,int size,int time_out);//成功返回0

};
void get_timestamp(HTimestamp *timestamp);
int64_th get_passtime(HTimestamp *timestamp);

void init_window_fifo(WindowFifo *fifo);

void close_protocol(HtProtocolContext *context);
int send_window(HtProtocolContext *context);
void check_write();

int write_pack_to_window(HtProtocolContext *context,void *buf,int size,int flag,int number,int is_first);
int read_pack_to_window(HtProtocolContext *context,int window_id,HtBuffer *recover_buf,int buf_offest,int size_offest,int update);

int read_pack(HtProtocolContext *context,HtBuffer *buf,int time_out);

//在队列中取出一个空余窗口
int get_free_window(WindowFifo *fifo);

int byte_stuffing(void *buf,int size,HtBuffer *buffer,int flag,int is_first);

// int message2pack(void *buf,int size,void *context);
void byte_stuffing_recover(HtBuffer *buffer,HtBuffer *buffer_res);
int write_respond(HtProtocolContext *context,int number,int flag);
void resend_nak(HtProtocolContext *context,uint8_th number);
void update_ack(HtProtocolContext *context,uint8_th number);
// HtProtocolContext *context;
void timer_clock(int pass_time_us);//时钟更新
int init_protocol_context(HtProtocolContext *context,int64_th retry_timeout_us);
int sendMessage(void *buf,int size,HtProtocolContext *context,int time_out);
int readMessage(void *buf,int size,HtProtocolContext *context,int time_out);

#endif

