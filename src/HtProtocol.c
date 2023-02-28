#include "HtProtocol.h"
#include "config.h"
int64_th time_cout_us=0;//定时器调用计数单位未毫秒 ms
HtProtocolContext *context_array[MAX_CONNECT]={0};//保存连接的context,以便遍历检查
int context_num=0;


void set_priv_data(HtProtocolContext *context,void *priv_data,int priv_size){
    context->priv_data=priv_data;
    context->priv_size=priv_size;
}


void show_window_check(WindowFifo *fifo,unsigned char *check_window,int window_max){

    printf("window number\n");
   
    for(int i=0;i<fifo->size;++i){
        int window_id=(fifo->head+i)%window_max;
        printf("%4d ",check_window[window_id]);
    }
    printf("\n");
}

void show_window_number(WindowFifo *fifo,int window_max){

    printf("window number\n");
   
    for(int i=0;i<fifo->size;++i){
        int window_id=(fifo->head+i)%window_max;
        printf("%4d ",fifo->fifo[window_id].number);
    }
    printf("\n");
}

void timer_clock(long long pass_time_us){
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
        check_window[i]=0;
    }
}

void init_window_fifo(WindowFifo *fifo){
    fifo->head=0;
    fifo->size=0;
}
int init_protocol_context(HtProtocolContext *context,long long retry_timeout_us){
    int i;
    context->pack_idx=0;
    context->activate=1;
    context->retry_timeout_us=retry_timeout_us;
    context->cp_pack_number=-1;
    init_window_fifo(&context->write_fifo);
    init_window_check(context->write_check,WINDOW_SIZE);
    init_window_fifo(&context->read_fifo);
    init_window_check(context->read_check,READ_WINDOW_SIZE);
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
    context->activate=0;
}


//在队列中取出一个空余窗口
int get_free_window(WindowFifo *fifo){
    int window_id;
    if(fifo->size == WINDOW_SIZE){
        return -1;//窗口已满
    }
    window_id=(fifo->head+fifo->size)%WINDOW_SIZE;
    fifo->size=fifo->size+1;
    return window_id;
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
        printf("all pack done\n");
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
    int i;
    int window_id=0;
    int size=context->read_fifo.size;
    for(i=0;i<size;i++){
        window_id=(context->read_fifo.head+i)%READ_WINDOW_SIZE;
        if(!context->read_check[window_id]){
            return 0;
        }
    }
    return 1;
}

// int replace_queue(WindowFifo *fifo,uint8_th *check,int check_state,int insert_idx,HtBuffer *buf){
int replace_queue_from_user(WindowFifo *fifo,uint8_th *check,int check_state,int insert_offest,void *user_buf,int size,int number){
    HtBuffer *buf_ptr;
    int window_idx;
    int last_number;
    int i=0;
    if(insert_offest>READ_WINDOW_SIZE-1)return -1;
    //计算要增加fifo长度还是不需要增加
    if(insert_offest>=fifo->size){//需要增加
        last_number=fifo->fifo[(fifo->head+fifo->size-1)%READ_WINDOW_SIZE].number;
        for(i=0;i<insert_offest-fifo->size;++i){
            // window_idx=(fifo->size+i)%READ_WINDOW_SIZE;
            window_idx=(fifo->head+fifo->size+i)%READ_WINDOW_SIZE;
            last_number=(last_number+1)%READ_WINDOW_SIZE;
            check[window_idx]=0;
            fifo->fifo[window_idx].number=last_number;//debug
        }
        fifo->size = insert_offest+1;
    }
    window_idx=(fifo->head+insert_offest)%READ_WINDOW_SIZE;
    buf_ptr=(fifo->fifo+window_idx);
    // if(size>READ_WINDOW_SIZE)return -1;
    // fifo->fifo[insert_idx]=*buf;
    
    memcpy(buf_ptr->buf,user_buf,size);
    buf_ptr->size=size;
    buf_ptr->number=number;
    check[window_idx]=check_state;
    return size;
}
int enqueue(WindowFifo *fifo,uint8_th *check,int check_state,HtBuffer *buf){
    int window_id;
    if( READ_WINDOW_SIZE == fifo->size)return -1;
    window_id=(fifo->head+fifo->size)%READ_WINDOW_SIZE;
    fifo->fifo[window_id]=*buf;
    check[window_id]=check_state;
    fifo->size=(fifo->size+1)%READ_WINDOW_SIZE;
    return buf->size;
}
int dequeue(WindowFifo *fifo,uint8_th *check,int check_state,HtBuffer *buf){
    HtBuffer *buf_ptr;
    if(fifo->size==0)return -1;
    buf_ptr= fifo->fifo + fifo->head;
    if(buf!=NULL)*buf=*buf_ptr;
    check[fifo->head]=check_state;//将该窗口接收标志修改为false,以便后面的包复用
    // if(!check_state)fifo->fifo[fifo->head].number=-1;//debug
    fifo->head=(fifo->head+1)%READ_WINDOW_SIZE;
    return buf->size;
}
int enqueue_from_user(WindowFifo *fifo,uint8_th *check,int check_state,void *user_buf,int size,int number){
    HtBuffer *buf_ptr;
    int window_id;
    if( READ_WINDOW_SIZE == fifo->size)return -1;
    window_id=(fifo->head + fifo->size)%READ_WINDOW_SIZE;
    buf_ptr= fifo->fifo + window_id;
    memcpy(buf_ptr->buf,user_buf,size);
    buf_ptr->number=number;
    buf_ptr->size=size;
    check[window_id]=check_state;
    fifo->size=fifo->size+1;
    return buf_ptr->size;
}
int dequeue_to_user(WindowFifo *fifo,uint8_th *check,int check_state,void *user_buf,int size){
    // static int dequeue_size=0;
    // printf("dequeue_to_user\n");
    HtBuffer *buf_ptr;
    if(fifo->size==0)return -1;
    if(fifo->size>size)return -1;
    buf_ptr= fifo->fifo + fifo->head;
    if(user_buf!=NULL)memcpy(user_buf,buf_ptr->buf,buf_ptr->size);
    check[fifo->head]=check_state;//将该窗口接收标志修改为false,以便后面的包复用
    // if(!check_state)fifo->fifo[fifo->head].number=-1;//debug
    fifo->head=(fifo->head+1)%READ_WINDOW_SIZE;
    fifo->size=fifo->size-1;
    // dequeue_size+=buf_ptr->size;
    // printf("\n\n");
    // for(int i=0;i<buf_ptr->size;++i){
    //     printf("%x ",buf_ptr->buf[i]);
    // }
    // printf("\n\n");
    // printf("dequeue_size %d\n",dequeue_size);
    return buf_ptr->size;
}


