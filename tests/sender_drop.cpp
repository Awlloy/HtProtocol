extern "C" {
#include "HtProtocol.h"
}
// #include <sys/timerfd.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>


#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <iostream>
#include <thread>
#include <sys/fcntl.h>
#include <atomic>
#define PORT 4567
#define PORT_RE 1234

#define IP "127.0.0.1"

std::atomic_bool close_thread={false};
std::atomic_bool thread_is_close={false};
static int setsocketnonblock(int sfd)
{
    int flags;
 
    flags = fcntl(sfd, F_GETFL, 0);
    if (flags < 0) {
        return -1;
    }
 
    return fcntl(sfd, F_SETFL, flags | O_NONBLOCK);
}

HtBuffer write_write;
int  sockfd;
int  sockfd_receive;
struct sockaddr_in my_addr;
struct sockaddr_in get_addr;
// HtBuffer write_read;
socklen_t len;

// HtBuffer read_write;
int writer(void *buf,int size,int time_out){
    // memcpy(write_write.buf,buf,size);
    // write_write.size=size;
    int scale=rand()%10;//a属于 [0,10)  rand（）使
    if(scale==1){
        return size;
    }   
    if(scale==5){
        size-=1;
    }else if(scale==7){
        size-=scale;
        if(size<1)size=1;
    }
    int si=sendto(sockfd, buf, size, 0, (sockaddr*)&my_addr, sizeof(my_addr));
    // printf("send size  %d %d \n",si,size);
    // printf("\n");
    // for(int i=0;i<si;++i){
    //     printf("%x ",((uint8_t*)buf)[i]);
    // }
    // printf("\n");
    return si;
}
int reader(void *buf,int size,int time_out){
    // memcpy(buf,read_write.buf,read_write.size);
    int si=recvfrom(sockfd_receive,buf,size,0,(struct sockaddr *)&get_addr,&len);
    // if(si!=-1)printf("reader get size  %d \n",si);

    // return read_write.size;
    return si;
    // return 0;
}



int main(){
    int ret;
    srand(time(0));
    
	// sockfd = socket(AF_INET, SOCK_STREAM, 0);
	// sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);//tcp
	sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);//udp
	sockfd_receive = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);//udp
	// sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);//raw   IPPROTO_TCP/IPPROTO_UDP
	if (sockfd==0)
    {
        std::cout << "create socket error !" << std::endl;
        return 0;
    }
    if (sockfd_receive==0)
    {
        std::cout << "create socket error !" << std::endl;
        return 0;
    }
    // printf("%d  %d\n",sockfd,sockfd_receive);
    // getchar();
    memset(&my_addr, 0, sizeof(my_addr));
	my_addr.sin_family = AF_INET; /* host byte order */
	my_addr.sin_port = htons(PORT); /* short, network byte order */
	my_addr.sin_addr.s_addr = inet_addr(IP);

    // memset(&get_addr, 0, sizeof(get_addr));
	get_addr.sin_family = AF_INET; /* host byte order */
	get_addr.sin_port = htons(PORT_RE); /* short, network byte order */
	get_addr.sin_addr.s_addr = inet_addr(IP);
    
   

	ret=bind(sockfd_receive, (struct sockaddr *)&get_addr, sizeof(get_addr));
    
    struct timeval tv;
    tv.tv_sec  = 0;
    tv.tv_usec = 1;  // 1 us
    setsockopt(sockfd_receive, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(struct timeval));
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(struct timeval));

    setsocketnonblock(sockfd);
    setsocketnonblock(sockfd_receive);

    HtProtocolContext ht_write;
    ht_write.read=reader;
    ht_write.write=writer;
    init_protocol_context(&ht_write,1000*100);//100ms

    std::thread thread([](){
        //启动一个线程更新时间
        struct timeval start;
        struct timeval end;
        int64_t pass_time_us=0;
        gettimeofday(&start,NULL);
        while(!close_thread.load()){
            gettimeofday(&end,NULL);
            int64_t timestamp=(end.tv_sec-start.tv_sec)*1000*1000 + (end.tv_usec-start.tv_usec);//us
            timer_clock(timestamp-pass_time_us);
            pass_time_us=timestamp;
            usleep(1000);
        }
        thread_is_close=true;
        
    });

    thread.detach();
    uint8_t write_buffer[1024]="12345 99dksfndskjfkhl23489dsmfdskhfdhsfhskklsdfhdahflkdsahfdkjsah fxcvnmvsdf i23uyr827o3irisdbfsdfdsfsdfsdfsdf0453 12312321232131237349543759834758937598347985743985392347jksdfhdhjsdhfdshfsdhfsadasdkashdjashdsadhjasdskahdsadhad475893475379485734895379534754398347598437439875934871123 hello world";
    printf("size %d\n",strlen((char *)write_buffer));
    // int send_size = sendMessage(write_buffer,strlen((char *)write_buffer),&ht_write,1000*1000*10);//10s
    int send_size = sendMessage(write_buffer,1024,&ht_write,1000*1000*10);//10s
    printf("send \n%s\n",write_buffer);
    printf("send_message %d\n",send_size);
    close_thread=true;
    while(!thread_is_close.load()){
        usleep(1000*10);
    }
    // uint8_t read_buffer[1024];
    // ret=readMessage(read_buffer,sizeof(read_buffer)-1,&ht_write,1000*1000*10);//10s
    // if(ret!=-1){
    //     read_buffer[ret]='\0';
    //     printf("read %s\n",read_buffer);
    // }
    // printf("read done\n");
    usleep(1000*10);
    return 0;
}