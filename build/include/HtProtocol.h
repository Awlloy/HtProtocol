                                                        
#ifndef HTPROTOCOL_H
#define HTPROTOCOL_H

#include <malloc.h>
#include <stdio.h>

typedef struct HtContext HtContext;
typedef struct PrivData PrivData;
struct HtContext{
    // char pad[CONTEXT_SIZE];
    char pad[1024*10];
};
struct PrivData{
    void *priv;
    int size;
};
typedef int (*HtReadwriteFunc)(void *buf,int size,int time_out,PrivData *priv);//成功返回0
void timer_clock(long long pass_time_us);//时钟更新

// int init_protocol_context(HtProtocolContext *context,long long retry_timeout_us);
// void set_priv_data(HtProtocolContext *context,void *priv_data,int priv_size);
// int sendMessage(void *buf,int size,HtProtocolContext *context,int time_out);
// int readMessage(void *buf,int size,HtProtocolContext *context,int time_out);

int init_protocol_context(void *context,long long retry_timeout_us);
void set_priv_data(void *context,void *priv_data,int priv_size);
void set_write_func(void *context,HtReadwriteFunc func);
void set_read_func(void *context,HtReadwriteFunc func);
int sendMessage(void *buf,int size,void *context,int time_out);
int readMessage(void *buf,int size,void *context,int time_out);
#endif

