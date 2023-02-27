#include "HtProtocol.h"



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
void init_window_check(uint8_th *check_window,int size){
    int i=0;
    for(i=0;i<size;++i){
        check_window[i]=false;
    }
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
    context->cp_pack_number=-1;
    init_window_fifo(&context->write_fifo);
    init_window_check(context->write_check,WINDOW_SIZE);
    init_window_fifo(&context->read_fifo);
    init_window_check(context->read_check,WINDOW_SIZE);
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
    // printf("byte_stuffing while done\n");
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
int check_read_window(HtProtocolContext *context){
    int ic=context->read_fifo.head;
    int rear=context->read_fifo.rear;
    //修改为已确认
    for(;ic!=rear;ic=(ic+1)%WINDOW_SIZE){
        if(!context->read_check[ic]){
            return 0;
        }
    }
    return 1;
}

// int replace_queue(WindowFifo *fifo,uint8_th *check,int check_state,int insert_idx,HtBuffer *buf){
int replace_queue_from_user(WindowFifo *fifo,uint8_th *check,int check_state,int insert_idx,void *user_buf,int size,int number){
    HtBuffer *buf_ptr=(fifo->fifo+insert_idx);
    if(size>WINDOW_SIZE)return -1;
    // fifo->fifo[insert_idx]=*buf;
    memcpy(buf_ptr->buf,user_buf,size);
    buf_ptr->size=size;
    buf_ptr->number=number;
    check[insert_idx]=check_state;
    return size;
}
int enqueue(WindowFifo *fifo,uint8_th *check,int check_state,HtBuffer *buf){
    if((fifo->rear+1)%WINDOW_SIZE == fifo->head)return -1;
    fifo->fifo[fifo->rear]=*buf;
    check[fifo->rear]=check_state;
    fifo->rear=(fifo->rear+1)%WINDOW_SIZE;
    return buf->size;
}
int dequeue(WindowFifo *fifo,uint8_th *check,int check_state,HtBuffer *buf){
    HtBuffer *buf_ptr;
    if(fifo->head== fifo->rear)return -1;//无数据
    buf_ptr= fifo->fifo + fifo->head;
    if(buf!=NULL)*buf=*buf_ptr;
    check[fifo->head]=check_state;//将该窗口接收标志修改为false,以便后面的包复用
    fifo->head=(fifo->head+1)%WINDOW_SIZE;
    return buf->size;
}
int enqueue_from_user(WindowFifo *fifo,uint8_th *check,int check_state,void *user_buf,int size,int number){
    HtBuffer *buf_ptr;
    if((fifo->rear+1)%WINDOW_SIZE == fifo->head)return -1;
    buf_ptr= fifo->fifo + fifo->rear;
    memcpy(user_buf,buf_ptr->buf,buf_ptr->size);
    buf_ptr->number=number;
    buf_ptr->size=size;
    check[fifo->rear]=check_state;
    fifo->rear=(fifo->rear+1)%WINDOW_SIZE;
    return buf_ptr->size;
}
int dequeue_to_user(WindowFifo *fifo,uint8_th *check,int check_state,void *user_buf,int size){
    HtBuffer *buf_ptr;
    if(fifo->head== fifo->rear)return -1;//无数据
    buf_ptr= fifo->fifo + fifo->head;
    if(user_buf!=NULL)memcpy(user_buf,buf_ptr->buf,buf_ptr->size);
    check[fifo->head]=check_state;//将该窗口接收标志修改为false,以便后面的包复用
    fifo->head=(fifo->head+1)%WINDOW_SIZE;
    return buf_ptr->size;
}


