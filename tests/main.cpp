#include "HtProtocol.h"
// #include <sys/timerfd.h>
#include <unistd.h>
#include <sys/time.h>
HtBuffer write_write;
// HtBuffer write_read;

// HtBuffer read_write;
HtBuffer read_write;
int write_writer(void *buf,int size,int time_out){
    memcpy(write_write.buf,buf,size);
    write_write.size=size;
    return size;
}
int write_reader(void *buf,int size,int time_out){
    memcpy(buf,read_write.buf,read_write.size);
    return read_write.size;
}

int read_writer(void *buf,int size,int time_out){
    memcpy(read_write.buf,buf,size);
    read_write.size=size;
    return size;
}
int read_reader(void *buf,int size,int time_out){
    memcpy(buf,write_write.buf,write_write.size);
    return write_write.size;
}

int main(){
    HtProtocolContext ht_write;
    init_protocol_context(&ht_write,1000);//1ms
    ht_write.read=write_reader;
    ht_write.write=write_reader;
    HtProtocolContext ht_read;
    init_protocol_context(&ht_read,1000);//1ms
    ht_read.read=read_reader;
    ht_read.write=read_writer;
    struct timeval start;
    struct timeval end;
    gettimeofday(&start,NULL);
    int8_th pass_time_us=0;
    while(1){
        gettimeofday(&end,NULL);
        int8_th timestamp=(end.tv_sec-start.tv_sec)*1000*1000 + (end.tv_usec-start.tv_usec);//us
        timer_clock(timestamp-pass_time_us);
        pass_time_us=timestamp;
        usleep(1000);
    }
    return 0;
}