#ifndef CONFIG__H
#define CONFIG__H

#include "HtProtocol.h"
#include <stdio.h>
#include "val_data.h"

// #define PACK_SIZE 128


int dequeue(WindowFifo *fifo,uint8_th *check,int check_state,HtBuffer *buf);
int enqueue(WindowFifo *fifo,uint8_th *check,int check_state,HtBuffer *buf);
int replace_queue_from_user(WindowFifo *fifo,uint8_th *check,int check_state,int insert_idx,void *user_buf,int size,int number);
int dequeue_to_user(WindowFifo *fifo,uint8_th *check,int check_state,void *user_buf,int size);
int enqueue_from_user(WindowFifo *fifo,uint8_th *check,int check_state,void *user_buf,int size,int number);
// int write_pack_to_window(HtProtocolContext *context,void *buf,int size,int flag,int number,int is_first);
// int read_pack_to_window(HtProtocolContext *context,int window_id,HtBuffer *recover_buf,int buf_offest,int size_offest,int update);

void get_timestamp(HTimestamp *timestamp);
int64_th get_passtime(HTimestamp *timestamp);

void init_window_fifo(WindowFifo *fifo);

//在队列中取出一个空余窗口
int get_free_window(WindowFifo *fifo);

int byte_stuffing(void *buf,int size,HtBuffer *buffer,int flag,int is_first);
// int message2pack(void *buf,int size,void *context);
void byte_stuffing_recover(HtBuffer *buffer,HtBuffer *buffer_res);

int write_respond(HtProtocolContext *context,int number,int flag);
void resend_nak(HtProtocolContext *context,uint8_th number);
void update_ack(HtProtocolContext *context,uint8_th number);



void show_window_check(WindowFifo *fifo,unsigned char *check_window,int window_max);
void show_window_number(WindowFifo *fifo,int window_max);

void close_protocol(HtProtocolContext *context);
#endif