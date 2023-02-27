
#include "HtProtocol.h"
extern int context_num;
extern HtProtocolContext *context_array[MAX_CONNECT];//保存连接的context,以便遍历检查
int send_pack(HtProtocolContext *context,HtBuffer *send_buf,HTimestamp *write_timestamp){
    static HTimestamp timestamp;
    static int ret;
    //读时间戳
    get_timestamp(&timestamp);
    ret=context->write(send_buf->buf,
                            send_buf->size,context->retry_timeout_us);
    //写成功的话更新更新写时间戳
    if(ret>0)*write_timestamp=timestamp;
    return ret;

}
int send_window(HtProtocolContext *context){
    // printf("send_window \n");
    //发送窗口数据
    int ret=0;
    int ic;
    int all_done=1;
    int rear;
    HTimestamp timestamp;
    int64_th retry_timeout_us=context->retry_timeout_us;
    HTimestamp *writes_timestamp=context->write_timestamp;
    ic=context->write_fifo.head;
    rear=context->write_fifo.rear;
    for(;ic!=rear;ic=(ic+1)%WINDOW_SIZE){
        if(context->write_check[ic])continue;//已经确认，无需重发
        all_done=0;
        if(get_passtime(writes_timestamp+ic)>=retry_timeout_us){
            ret=send_pack(context,&(context->write_fifo.fifo[ic]),&writes_timestamp[ic]);
            //读时间戳
            // get_timestamp(&timestamp);
            // //重发帧
            // ret=context->write(context->write_fifo.fifo[ic].buf,
            //                 context->write_fifo.fifo[ic].size,retry_timeout_us);
            // if(ret==0)writes_timestamp[ic]=timestamp;
            if(ret==-1)return -1;
        }
    }
    if(all_done){
        // printf("window send done\n");
        return 1;
    }
    return 0;

}

void check_write(){
    //把剩余窗口的数据进行检查重发
    static int i=0;
    static HtProtocolContext *context=NULL;
    // HTimestamp timestamp;
    // get_timestamp(&timestamp);
    for(i=0;i<context_num;++i){
        context=context_array[i];
        if(context!=NULL && context->activate){
            send_window(context);
        }
    }
}
int write_pack_to_window(HtProtocolContext *context,int *_window_id,void *buf,int size,int flag,int number_,int is_first){//将用户数据放入发送窗口
    HtBuffer *window_buf;
    static int number=0;
    int new_rear;
    int window_id;
    int pack_size=0;
    if(size==0)return -1; 
    window_id=get_free_window(&(context->write_fifo));
    if(window_id==-1)return -1;//窗口已满，无法装入
    context->write_check[window_id]=false;
    window_buf=&(context->write_fifo.fifo[window_id]);
    window_buf->number=number;
    window_buf->size=PACK_SIZE;
    pack_size=byte_stuffing(buf,size,window_buf,flag,is_first);
    printf("number %d\n",number);
    *_window_id=window_id;
    ++number;
    //字节填充
    return pack_size;//数据完全装入窗口
}

void update_ack(HtProtocolContext *context,uint8_th number){
    int ic=context->write_fifo.head;
    int rear=context->write_fifo.rear;
    //修改为已确认
    for(;ic!=rear;ic=(ic+1)%WINDOW_SIZE){
        if(context->write_fifo.fifo[ic].number == number){
            context->write_check[ic]=true;
            break;
        }
    }
    //窗口移动
    ic=context->write_fifo.head;
    for(;ic!=rear;ic=(ic+1)%WINDOW_SIZE){
        if(!context->write_check[ic]){
            break;
        }
    }
    context->write_fifo.head=ic;
}
void resend_nak(HtProtocolContext *context,uint8_th number){
    int ic=context->write_fifo.head;
    int rear=context->write_fifo.rear;
    for(;ic!=rear;ic=(ic+1)%WINDOW_SIZE){
        if(context->write_fifo.fifo[ic].number == number){
            // context->write_check[ic]=false;
            context->write(context->write_fifo.fifo[ic].buf,context->write_fifo.fifo[ic].size,context->retry_timeout_us);
            break;
        }
    }
}
int write_respond(HtProtocolContext *context,int number,int flag){
    static HtBuffer send_buf;
    int buf_idx=0;
    uint8_th val_data;
    int ret;
    send_buf.buf[buf_idx++]=HEADFLAG;
    // send_buf.buf[buf_idx++]=flag | LF;
    send_buf.buf[buf_idx++]=flag;
    if(number==HEADFLAG || number==PADFLAG)send_buf.buf[buf_idx++]=PADFLAG;
    send_buf.buf[buf_idx++]=number;
    send_buf.buf[buf_idx++]=PADFLAG;
    val_data=get_val_sum(0,send_buf.buf,buf_idx);
    send_buf.buf[buf_idx++]=val_data;
    send_buf.size=buf_idx;
    printf("write_respond number %d   flag %x\n",number,flag);
    return context->write(send_buf.buf,
                        send_buf.size,
                        context->retry_timeout_us);
}
/**
 * 回退n帧协议
 * 选择重传
 * */
int sendMessage(void *buf,int size,HtProtocolContext *context,int time_out){
    /**
     * while(time<time_out){
     *      write()
     * 
     * }
     * 等待
     * 
     * 
     * 
     * ret -1  发送超时
     * */
    // static int number=0;
    int ret;
    HtBuffer read_buf;
    HtBuffer recover_buf;
    HTimestamp timestamp;
    uint8_th *buf_send;
    int size_send;
    int first;
    int flag;
    int window_id;
    uint8_th val_data;
    int send_done=0;
    // init_window_fifo(&context->write_fifo);
    get_timestamp(&timestamp);
    buf_send=(uint8_th *)buf;
    size_send=size;
    first=1;
    flag=0;

    while(get_passtime(&timestamp)<time_out){
        // printf("size_send %d\n",size_send);
        if(size_send>0){
            ret=write_pack_to_window(context,&window_id,buf_send,size_send,flag,1,first);
            // printf("write_pack_to_window %d %d ret\n",number,ret);
            first=0;
            //发送当前帧数据
            if(ret>=0){
                //装入部分数据到窗口，跳过已经装入数据
                buf_send+=ret;
                size_send-=ret;
                ret=send_pack(context,&(context->write_fifo.fifo[window_id]),&context->write_timestamp[window_id]);
                printf("send window size %d\n",size-size_send);
                if(ret==-1)break;//发送出错
            }
        }
        //打包数据到发送窗口
        ret=send_window(context);//检查超时并发送窗口数据
        if(ret==-1)break;//发送出错
        if(ret==1 && size_send==0 && !send_done){
            //数据发送完成并全部收到反馈，进入接收者结束信号，并超时等待
            printf("send done\n");
            send_done=1;
            time_out=context->retry_timeout_us;//修改超时等待反馈
            // rf_number=recover_buf.number;
            get_timestamp(&timestamp);
            // break;//发送完成
        }
        // if(ret==0 || ret==size_send)continue;//数据已经发送一轮完毕，目前还剩余窗口数据未确认完毕
        
        //回读用户反馈结果
        read_buf.size=PACK_SIZE;
        ret=read_pack(context,&read_buf,time_out);//读取一个帧
        // printf("ret %d\n",ret);
        if(ret==-1 || ret==0)continue;//接收出错或无结果
        // printf("writer get_val_sum \n");
        val_data=get_val_sum(0,read_buf.buf,read_buf.size-1);//校验
        // printf("writer val_data \n");
        if(val_data!=read_buf.buf[read_buf.size-1])continue;//校验不通过
        printf("writer val_data  done read_buf.buf[0] %x\n",read_buf.buf[0]);
        if(read_buf.buf[0]==HEADFLAG){//只看head包
            byte_stuffing_recover(&read_buf,&recover_buf);//解包数据
            flag=recover_buf.buf[1];//读取flag
            printf("writer get head  flag %x\n",flag);
            recover_buf.number=recover_buf.buf[2];
            // printf("flag %x %d\n",flag&ACK,(flag&ACK)==ACK);
            if((flag &(ACK|RF) == (ACK|RF)) && send_done){
                // return cp_pack_idx;
                write_respond(context,recover_buf.number,ACK|RF);//反馈收到结果，并结束发送
                printf("send RF done\n");
                return size;
            }
            else if((flag&ACK)==ACK){
                printf("ack recover_buf.number %d\n",recover_buf.number);
                update_ack(context,recover_buf.number);
                continue;
            }
            else if((flag&NAK)==NAK){
                printf("nak recover_buf.number %d\n",recover_buf.number);
                resend_nak(context,recover_buf.number);
                continue;
            }
        }
    }
    if(send_done){
        return size;
    }
    return -1;
}