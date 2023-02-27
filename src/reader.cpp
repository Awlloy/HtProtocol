
#include "HtProtocol.h"
#include "config.h"
extern int context_num;
extern HtProtocolContext *context_array[MAX_CONNECT];//保存连接的context,以便遍历检查

int read_pack(HtProtocolContext *context,HtBuffer *buf,int time_out);
int check_read_window(HtProtocolContext *context);


int read_pack(HtProtocolContext *context,HtBuffer *buf,int time_out){
    int ret=context->read(buf->buf,buf->size,time_out);
    buf->size=ret;
    return ret;
}
int get_pack_to_window_offest(HtProtocolContext *context,int number){
    //检查是否可以放入窗口,允许超出1个窗口,因为需要考虑是否可以挤调fifo head
    int offest;
    int head_number;
    if(context->read_fifo.size==0)return 0;//第一个,直接入队
    head_number=context->read_fifo.fifo[context->read_fifo.head].number;
    offest = number - head_number;
    if(offest<0)offest+=NUMBER_MAX_SIZE;
    //检查是否在可接收窗口内 //在可缓存的下标范围内,无需
    if( offest <= WINDOW_SIZE ){
        return offest;
    }
    return -1;
}

int readMessage(void *buf,int size,HtProtocolContext *context,int time_out){
    /**
     * while(time<time_out){
     *      read()
     * 
     * 
     * 记录head序号
     * 
     * 
     * 
     * 等待
     * 读x
     * 校验
     * 包类型划分
     * 
     * 数据包：检查是否重复，返回ACK，并检查x-1是否已经收到，否则发送确认
     * 确认包：将写缓冲区对应的序号制确认并head后移动
     * 重发包: 将写缓冲区对应的序号发送，并设置序号为未确认
     * 

     * 

     * head已经发送确认的话，将head数据包装入用户缓冲区，则接收窗口后移，即head=（head+1）%max_window
     * 
     * */
    uint8_th *user_buf=(uint8_th*)buf;
    int cp_size=size;
    HtBuffer read_buf;//
    HtBuffer recover_buf;//
    HtBuffer head_buf;//
    uint8_th val_data;
    uint8_th *decode_data;
    int decode_data_size;
    int ret;
    int flag;
    HTimestamp timestamp;
    int rec_size=0;
    int is_head=0;
    int is_rear=0;
    // int cp_pack_number=-1;//保存下一个装入用户缓冲区的序号
    int window_id_offest=-1;
    int window_id=-1;
    int rf_number=-1;
    int closing=0;
    head_buf.number=-1;
    head_buf.size=WINDOW_SIZE;
    init_window_fifo(&context->read_fifo);//初始化接收fifo
    get_timestamp(&timestamp);
    while(get_passtime(&timestamp)<time_out){
        is_head=0;
        // printf("%d\n",get_passtime(&timestamp));
        read_buf.size=PACK_SIZE;
        ret=read_pack(context,&read_buf,time_out);//读取一个帧
        if(ret==-1 || ret==0)continue;//接收出错或无结果
        val_data=get_val_sum(0,read_buf.buf,read_buf.size-1);//校验
        
        if(val_data!=read_buf.buf[read_buf.size-1])continue;//校验不通过

        if(read_buf.buf[0]==HEADFLAG){//第一个包
            is_head=1;
        }
        byte_stuffing_recover(&read_buf,&recover_buf);//解包数据
        if(is_head){//第一个包
            flag=recover_buf.buf[1];//读取flag
            recover_buf.number=recover_buf.buf[2];
            decode_data=recover_buf.buf+3;
            decode_data_size=recover_buf.size-4;
        }else{
            flag=recover_buf.buf[0];//读取flag
            recover_buf.number=recover_buf.buf[1];
            decode_data=recover_buf.buf+2;
            decode_data_size=recover_buf.size-3;
        }
        if(is_head){
            if((flag &(ACK|RF) == (ACK|RF))){
                if(closing)break;
            }else if((flag & RF) == (RF)){
                closing=1;
                write_respond(context,recover_buf.number,ACK|RF);//反馈收到结果，并结束发送
                time_out=context->retry_timeout_us * 2;//修改超时等待反馈
                get_timestamp(&timestamp);
                write_respond(context,recover_buf.number,RF);//发送关闭信号
                continue;
            }
            else if((flag&ACK)==ACK){
                update_ack(context,recover_buf.number);
                printf("ack recover_buf.number %d\n",recover_buf.number);
                continue;
            }
            else if((flag&NAK)==NAK){
                printf("nak recover_buf.number %d\n",recover_buf.number);
                resend_nak(context,recover_buf.number);
                continue;
            }
            else if(head_buf.number!=recover_buf.number && head_buf.buf[head_buf.size-1] != recover_buf.buf[recover_buf.size-1]){
                head_buf=recover_buf;//复制head包
                rec_size=0;
                user_buf=(uint8_th *)buf;
                cp_size=size;
                init_window_fifo(&(context->read_fifo));
                context->read_check[context->read_fifo.head]=false;
                is_rear=0;
            }
        }
        //计算新包的number在fifo中与head的偏移量,-1为无法缓存
        window_id_offest=get_pack_to_window_offest(context,recover_buf.number);
        if(window_id_offest<0){
            printf("无法装入窗口\n");
            continue;//无法装入窗口,
        }
        printf("head %d   window_id_offest %d recover_buf.number %d\n",context->read_fifo.fifo[context->read_fifo.head].number,window_id_offest,recover_buf.number);
        ret=-1;
        if(context->read_fifo.size==0){//第一帧
            printf("first\n");
            ret=enqueue_from_user(&(context->read_fifo),context->read_check,true,decode_data,decode_data_size,recover_buf.number);
        }else if(window_id_offest < WINDOW_SIZE){//未缓存该帧
            printf("second  %d\n",window_id_offest);
            window_id = (context->read_fifo.head+window_id_offest)%WINDOW_SIZE;//新帧在窗口的坐标
            if(!context->read_check[window_id]){//未缓存
                //缓存该帧
                ret=replace_queue_from_user(&(context->read_fifo),context->read_check,true,window_id_offest,decode_data,decode_data_size,recover_buf.number);
            }else{
                ret=0;//已有缓存
                printf("已有缓存 重复包\n");
            }
            
        }else if(window_id_offest==WINDOW_SIZE){
            printf("thirdly\n");
            //若number超出窗口范围则返回-1
            window_id = (context->read_fifo.head+window_id_offest)%WINDOW_SIZE;//新帧在窗口的坐标
            //检查能否
            if(context->read_check[context->read_fifo.head]){//头已经收到,考虑替换
                ret=dequeue_to_user(&(context->read_fifo),context->read_check,false,user_buf,cp_size);
                if(ret==-1)return -1;
                user_buf+=ret;
                cp_size-=ret;
                rec_size+=ret;
                ret=enqueue_from_user(&(context->read_fifo),context->read_check,true,decode_data,decode_data_size,recover_buf.number);
            }
        }
        if(ret>=0)write_respond(context,recover_buf.number,ACK);//装入缓冲区成功，反馈发送者
        if((flag&LF)==LF && ret >= 0){//rear帧成功放入缓存内
            is_rear=1;
            rf_number=recover_buf.number;//记录最后帧的序号，以便后面发送确认
        }
        // if(is_rear && check_read_window(context)){//所有包已经接收完成
        //     printf("all read done send out\n");
        //     write_respond(context,rf_number,ACK|RF);//装入缓冲区成功，反馈发送者
        //     time_out=context->retry_timeout_us * 2;//修改超时等待反馈
        //     get_timestamp(&timestamp);

        //     //拷贝数据

        // }
    }
    if(is_rear && check_read_window(context)){//所有包已经接收完成
        printf("all read done but receive rf time out %d\n",context->read_fifo.size);
        // for(){
        int i;
        int window_id=0;
        int size=context->read_fifo.size;
        for(i=0;i<size;i++){
            window_id=i%WINDOW_SIZE;
            ret=dequeue_to_user(&(context->read_fifo),context->read_check,false,user_buf,cp_size);
            if(ret==-1)return -1;
            user_buf+=ret;
            cp_size-=ret;
            rec_size+=ret;
        }
        printf("rec_size %d\n",rec_size);
        return rec_size;
    }
    return -1;
}
