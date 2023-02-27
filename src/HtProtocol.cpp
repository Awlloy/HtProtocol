#include "HtProtocol.h"
#define HTABS(x) ((x)>0)?(x):-(x)

enum HPFlag{
    ACK=0x40,//应答
    NAK=0x20,//请求
    LF=0x10,//最后一段
    RF=0x08,//最后一包接收者反馈
};

int64_th time_cout_us=0;//定时器调用计数单位未毫秒 ms
HtProtocolContext *context_array[MAX_CONNECT]={0};//保存连接的context,以便遍历检查
int context_num=0;




void timer_clock(int pass_time_us){
    time_cout_us+=pass_time_us;
    //check write
    // check_write();

}

void get_timestamp(HTimestamp *timestamp){
    timestamp->time_tamp=time_cout_us;
}
int64_th get_passtime(HTimestamp *timestamp){
    return time_cout_us-timestamp->time_tamp;
}

void init_window_fifo(WindowFifo *fifo){
    fifo->head=0;
    fifo->rear=0;
}
int init_protocol_context(HtProtocolContext *context,int64_th retry_timeout_us){
    int i;
    context->pack_idx=0;
    context->activate=true;
    context->retry_timeout_us=retry_timeout_us;
    context->cp_pack_number=0;
    init_window_fifo(&context->write_fifo);
    init_window_fifo(&context->read_fifo);
    for(i=0;i<MAX_CONNECT;++i){
        if(context_array[i]==NULL || !(context_array[i]->activate)){
            context_array[i]=context;
            ++context_num;
            return i;//返回存储索引
        }
    }
    return -1;//无法存储该context
}
void close_protocol(HtProtocolContext *context){
    context->activate=false;
}
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
int read_pack_to_window(HtProtocolContext *context,HtBuffer *recover_buf,int buf_offest,int size_offest){//将读取到的数据装入到读取数据窗口
    HtBuffer *window_buf;
    int window_id;
    int i;
    int head=context->read_fifo.head;
    int rear=context->read_fifo.rear;
    int offest;
    
    context->read_check[context->read_fifo.head]=false;//头帧标记为未收到
    //检查是否在可接收窗口内
    if((recover_buf->number - context->cp_pack_number)<WINDOW_SIZE  
    || (recover_buf->number - context->cp_pack_number + NUMBER_MAX_SIZE ) < WINDOW_SIZE){
        //在可缓存范围
        offest = recover_buf->number - context->cp_pack_number;
        if(offest<0)offest+=NUMBER_MAX_SIZE;
        window_id = (context->read_fifo.head+offest)%WINDOW_SIZE;//新帧在窗口的坐标
        //判断该帧是否已经开辟过位置
        if(HTABS(rear - head - 1) < offest){//需要开辟窗口,从对尾开始开辟
            for(i=rear;i<=window_id;i=(i+1)%WINDOW_SIZE){
                context->read_check[i]=false;
            }
            context->read_fifo.rear=i;//更新队尾
        }
        if(!context->read_check[window_id]){//未缓存该帧
            memcpy(context->read_fifo.fifo[window_id].buf,recover_buf->buf+buf_offest,recover_buf->size+size_offest);
            context->read_fifo.fifo[window_id].size=recover_buf->size+size_offest;
            context->read_fifo.fifo[window_id].number=recover_buf->number;
            context->read_check[window_id]=true;//更新为已缓存
        }
        return 0;//缓存成功
    }
    return -1;
}

int read_pack(HtProtocolContext *context,HtBuffer *buf,int time_out){
    int ret=context->read(buf->buf,buf->size,time_out);
    buf->size=ret;
    return ret;
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


int read_pack_from_window(HtProtocolContext *context,HtBuffer *recover_buf,int buf_offest,int size_offest){//将读取到的数据装入到读取数据窗口
    HtBuffer *window_buf;
    int window_id;
    int i;
    int head=context->read_fifo.head;
    int rear=context->read_fifo.rear;
    int offest;
    
    context->read_check[context->read_fifo.head]=false;//头帧标记为未收到
    //检查是否在可接收窗口内
    if((recover_buf->number - context->cp_pack_number)<WINDOW_SIZE  
    || (recover_buf->number - context->cp_pack_number + NUMBER_MAX_SIZE ) < WINDOW_SIZE){
        //在可缓存范围
        offest = recover_buf->number - context->cp_pack_number;
        if(offest<0)offest+=NUMBER_MAX_SIZE;
        window_id = (context->read_fifo.head+offest)%WINDOW_SIZE;//新帧在窗口的坐标
        //判断该帧是否已经开辟过位置
        if(HTABS(rear - head - 1) < offest){//需要开辟窗口,从对尾开始开辟
            for(i=rear;i<=window_id;i=(i+1)%WINDOW_SIZE){
                context->read_check[i]=false;
            }
            context->read_fifo.rear=i;//更新队尾
        }
        if(!context->read_check[window_id]){//未缓存该帧
            memcpy(context->read_fifo.fifo[window_id].buf,recover_buf->buf+buf_offest,recover_buf->size+size_offest);
            context->read_fifo.fifo[window_id].size=recover_buf->size+size_offest;
            context->read_fifo.fifo[window_id].number=recover_buf->number;
            context->read_check[window_id]=true;//更新为已缓存
        }
        return 0;//缓存成功
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
    HtBuffer read_buf;//
    HtBuffer recover_buf;//
    HtBuffer head_buf;//
    HtBuffer *buf_ptr;//
    uint8_th val_data;
    int ret;
    int flag;
    HTimestamp timestamp;
    
    int is_head=0;
    int is_rear=0;
    int ic,rear;
    int cp_pack_idx=0;
    uint8_th *cp_buf_ptr;
    int cp_pack_number=-1;//保存下一个装入用户缓冲区的序号
    int cp_size;
    int rf_number=-1;
    head_buf.number=-1;
    head_buf.size=WINDOW_SIZE;
    init_window_fifo(&context->read_fifo);//初始化接收fifo
    get_timestamp(&timestamp);
    while(get_passtime(&timestamp)<time_out){
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
        }else{
            flag=recover_buf.buf[0];//读取flag
            recover_buf.number=recover_buf.buf[1];
        }
        // printf("is_head %d val_data pass number %d %d %d\n",is_head,recover_buf.number,recover_buf.buf[0],recover_buf.buf[2]);
        if(is_head){
            if((flag &(ACK|RF) == (ACK|RF)) && rf_number==recover_buf.number){
                return cp_pack_idx;
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
                cp_pack_idx=0;
                context->cp_pack_number=recover_buf.number;
                context->read_fifo.head=context->read_fifo.rear;
            }
        }
        if((flag&LF)==LF){
            is_rear=1;
            rf_number=recover_buf.number;//记录最后帧的序号，以便后面发送确认
        }
        printf("is_head %d  is_rear %d number %d\n",is_head,is_rear,recover_buf.number);
        // printf("is_head  %d\n",is_head);
        // }
        //检查包是否已经接收过   已接收 返回ACK
        //检查包是否连续，
        //若连续则直接 装入用户数据，返回ACK
        //不连续，数据装入接收队列
        ret=0;
        // printf(" context->cp_pack_number %d\n", context->cp_pack_number);
        if(recover_buf.number != context->cp_pack_number){//出现顺序错误包
        /**
         * 可能出现序号情况  收到     目标      是否缓存
         *                  1       253         是
         *                  11       5         是
         *                  253       1         否
         *                                              1    -     253 + 256  
         * (number - cp_pack_number)<window_size  || (number - cp_pack_number + NUMBER_MAX_SIZE ) < window_size
         * 
         * */
            // printf("read move read_pack_to_window\n");

            //先缓存该包
            // write_check[]
            if(is_head){
                // ret=read_pack_to_window(context,&recover_buf,3,4);//窗口无法移动，尝试将数据放入接收缓冲
                ret=read_pack_to_window(context,&recover_buf,3,-4);//窗口无法移动，尝试将数据放入接收缓冲
            }else{//
                // ret=read_pack_to_window(context,&recover_buf,2,3);//窗口无法移动，尝试将数据放入接收缓冲
                ret=read_pack_to_window(context,&recover_buf,2,-3);//窗口无法移动，尝试将数据放入接收缓冲
            }
            printf("read move read_pack_to_window done\n");
            write_respond(context,context->cp_pack_number,NAK);//主动请求重发队头数据
        }else{  
            printf("read move window\n");
            // pack_to_user_buf()
            //窗口可移动 + 1
            if(is_head){
                // ret=read_pack_from_window(context,&recover_buf,3,4);
                cp_buf_ptr=recover_buf.buf+3;//跳过head和flag和number
                cp_size=recover_buf.size-4;//多跳过最后的校验
            }else{
                // ret=read_pack_from_window(context,&recover_buf,1,2);

                cp_buf_ptr=recover_buf.buf+2;//跳过number
                cp_size=recover_buf.size-3;//多跳过最后的校验
            }
            if(cp_pack_idx+cp_size > size)return -1;//用户缓冲不足
            memcpy(user_buf+cp_pack_idx,cp_buf_ptr,cp_size);
            cp_pack_idx+=cp_size;
            context->read_check[context->read_fifo.head]=true;//读取到需要帧
            //拷贝数据
            //窗口移动 1
            context->cp_pack_number++;//
            context->read_fifo.head=(context->read_fifo.head+1)%WINDOW_SIZE;
            context->read_fifo.rear=(context->read_fifo.rear+1)%WINDOW_SIZE;

            // context->read_fifo.rear=()%WINDOW_SIZE;
            // if(!is_rear){
            //     //检查接收缓冲区，将提前收到的数据也一并拷贝到用户
            int ic=context->read_fifo.head;
            int rear=context->read_fifo.rear;
            for(;ic!=rear;ic=(ic+1)%WINDOW_SIZE){
                if(context->read_fifo.fifo[ic].number == context->cp_pack_number
                    && context->read_check[ic]){//已经接收过，可以写入
                    buf_ptr=&(context->read_fifo.fifo[ic]);
                    if(cp_pack_idx+buf_ptr->size > size)return -1;//缓冲不足
                    memcpy(user_buf+cp_pack_idx,buf_ptr->buf,buf_ptr->size);
                    cp_pack_idx+=buf_ptr->size;
                    context->cp_pack_number++;
                    continue;
                }
                break;
            }
            context->read_fifo.head=ic;
            printf("copy size %d\n",cp_pack_idx);
        }
        if(ret>=0)write_respond(context,recover_buf.number,ACK);//装入缓冲区成功，反馈发送者
        if(is_rear){
            printf("is_rear\n");
        }
        if(is_rear && context->read_fifo.head==context->read_fifo.rear){//所有包已经接收完成
            printf("all read done\n");
            write_respond(context,rf_number,ACK|RF);//装入缓冲区成功，反馈发送者
            time_out=context->retry_timeout_us * 2;//修改超时等待反馈
            // rf_number=recover_buf.number;
            get_timestamp(&timestamp);

        }
        is_head=0;//取消标记is_head
    }
    if(is_rear && context->read_fifo.head==context->read_fifo.rear){//所有包已经接收完成
        printf("all read done but receive rf time out\n");
        return cp_pack_idx;
    }
    return -1;
}

//在队列中取出一个空余窗口
int get_free_window(WindowFifo *fifo){
    int rear=fifo->rear;
    int new_rear = (rear+1)%WINDOW_SIZE;
    if( new_rear == fifo->head){
        return -1;//窗口已满
    }
    fifo->rear=new_rear;
    return rear;
}

int byte_stuffing(void *buf,int size,HtBuffer *buffer,int flag,int is_first){
    //is_first  在头部填充一个flag
    int buf_size=0;
    int write_buf_idx=0;
    uint8_th *write_buf=(uint8_th *)buf;
    uint8_th *buf_ptr=buffer->buf;
    uint8_th val_data;
    // printf("size %d\n",size);

    if(is_first>0){
        buf_ptr[buf_size++]=HEADFLAG;
    }
    buf_ptr[buf_size++]=flag;
    if(buffer->number==HEADFLAG || buffer->number==PADFLAG)buf_ptr[buf_size++]=PADFLAG;//序号可能和Flag重叠，需要检查并加
    buf_ptr[buf_size++]=buffer->number;

    while(buf_size<PACK_SIZE-2 && write_buf_idx<size){
        if(write_buf[write_buf_idx]==HEADFLAG || write_buf[write_buf_idx]==PADFLAG){
            //填充一个 PADFLAG
            buf_ptr[buf_size++]=PADFLAG;
        }
        buf_ptr[buf_size++]=write_buf[write_buf_idx++];
    }
    printf("byte_stuffing while done\n");
    // if(is_first>0 && write_buf_idx==size){//数据全部打包完毕
    if(write_buf_idx==size){//数据全部打包完毕
        if(is_first>0){
            buf_ptr[1] |= LF;
        }else{
            buf_ptr[0] |= LF;
        }
        // flag|=LF;//最后段标志位设置
    }
    buf_ptr[buf_size++]=PADFLAG;//在校验前填充一个，以便防止校验结果为PADFALG或在HEADFLAG
    // printf("buf_size %d\n",buf_size);
    val_data=get_val_sum(0,buf_ptr,buf_size);
    // printf("get_val_sum done\n");
    buf_ptr[buf_size++]=val_data;
    buffer->size=buf_size;
    return write_buf_idx;
}
void byte_stuffing_recover(HtBuffer *buffer,HtBuffer *buffer_res){
    // static HtBuffer buffer;
    int i;
    int res_idx=0;
    for(i=0;i<buffer->size;++i){
        if(buffer->buf[i]==PADFLAG)++i;
        buffer_res->buf[res_idx++]=buffer->buf[i];
    }
    buffer_res->size=res_idx;
}



