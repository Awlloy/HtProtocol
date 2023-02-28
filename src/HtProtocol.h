                                                        
#ifndef HTPROTOCOL_H
#define HTPROTOCOL_H
#include "val_data.h"
#include <malloc.h>
#include <stdio.h>
//窗口大小，窗口越大占用空间越大
#define WINDOW_SIZE 15
// #define READ_WINDOW_SIZE 30
#define READ_WINDOW_SIZE 30
#define PACK_SIZE 20


typedef struct WindowFifo WindowFifo;
typedef struct HtProtocolContext HtProtocolContext;
typedef struct HtBuffer HtBuffer;
typedef struct HTimestamp HTimestamp;

// extern int64_th time_cout_us;
// extern HtProtocolContext *context_array[MAX_CONNECT];//保存连接的context,以便遍历检查
// extern int context_num;

struct HtBuffer{
    uint8_th buf[PACK_SIZE];
    uint16_th size;
    // int number;
    uint8_th number;

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
    uint16_th head;//head为第一个数据
    uint16_th size;//fifo占用长度
};
struct HTimestamp{
    int64_th time_tamp;
};
struct HtProtocolContext{
    uint16_th pack_idx;
    int64_th retry_timeout_us;
    //滑动窗口
    // uint16_th read_idx_head;
    // uint16_th read_idx_rear;
    WindowFifo write_fifo;
    uint8_th write_check[WINDOW_SIZE];
    HTimestamp write_timestamp[WINDOW_SIZE];

    WindowFifo read_fifo;
    uint8_th read_check[READ_WINDOW_SIZE];
    
    int cp_pack_number;
    int8_th activate;
    int (*read)(void *buf,int size,int time_out);//成功返回0
    int (*write)(void *buf,int size,int time_out);//成功返回0

};
void show_window_check(WindowFifo *fifo,uint8_th *check_window,int window_max);
void show_window_number(WindowFifo *fifo,int window_max);

void timer_clock(int pass_time_us);//时钟更新
void close_protocol(HtProtocolContext *context);
int init_protocol_context(HtProtocolContext *context,int64_th retry_timeout_us);
int sendMessage(void *buf,int size,HtProtocolContext *context,int time_out);
int readMessage(void *buf,int size,HtProtocolContext *context,int time_out);

#endif

