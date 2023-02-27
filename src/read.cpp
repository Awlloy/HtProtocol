
#include "HtProtocol.h"
extern int context_num;
extern HtProtocolContext *context_array[MAX_CONNECT];//保存连接的context,以便遍历检查

int read_pack(HtProtocolContext *context,HtBuffer *buf,int time_out){
    int ret=context->read(buf->buf,buf->size,time_out);
    buf->size=ret;
    return ret;
}
int get_pack_to_window_offest(HtProtocolContext *context,int number){
    int head=context->read_fifo.head;
    int rear=context->read_fifo.rear;
    int offest;
    int head_number=context->read_fifo.fifo[head].number;
    offest = number - head_number;
    if(offest<0)offest+=NUMBER_MAX_SIZE;
    if(head==rear)return 0;//
    //检查是否在可接收窗口内 //在可缓存的下标范围内,无需
    if( offest < WINDOW_SIZE ){
        return offest;
    }
    return -1;
}

int read_pack_to_window_rear(HtProtocolContext *context,HtBuffer *recover_buf,int buf_offest,int size_offest){

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
    int cp_size=size;
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
    int window_id_offest=-1;
    int window_id=-1;
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
                init_window_fifo(&(context->read_fifo));
                context->read_check[context->read_fifo.head]=false;
                is_rear=0;
            }
        }
        printf("is_head %d  is_rear %d number %d\n",is_head,is_rear,recover_buf.number);
        // printf("is_head  %d\n",is_head);
        // }
        //检查包是否已经接收过   已接收 返回ACK
        //检查包是否连续，
        //若连续则直接 装入用户数据，返回ACK
        //不连续，数据装入接收队列
        // printf(" context->cp_pack_number %d\n", context->cp_pack_number);
        //计算新包的number在fifo中与head的偏移量,-1为无法缓存
        window_id_offest=get_pack_to_window_offest(context,recover_buf.number);
        if(window_id_offest<0){
            printf("无法装入窗口");
            continue;//无法装入窗口,
        }
        //若number超出窗口范围则返回-1
        ret=0;
        window_id = (context->read_fifo.head+window_id_offest)%WINDOW_SIZE;//新帧在窗口的坐标
        if(!context->read_check[window_id]){//未缓存该帧
            //检查能否
            if(context->read_check[context->read_fifo.head]){//头已经收到,考虑替换
                if(window_id_offest == WINDOW_SIZE - 1 
                && ((context->read_fifo.rear+1)%WINDOW_SIZE == context->read_fifo.head)){//最后一个下标,当head已经接收过时,需要将head移动.即替换head,过滤最旧已经接收到的数据数据
                    //直接出队入队
                    //并且将head包拷贝到用户数据
                    ret=dequeue_to_user(&(context->read_fifo),context->read_check,false,user_buf,cp_size);
                    if(ret==-1)return -1;
                    user_buf+=ret;
                    cp_size-=ret;

                    // if(is_head){//缓存该帧
                    //     ret=enqueue_from_user(&(context->read_fifo),context->read_check,true,recover_buf.buf+3,recover_buf.size-4,recover_buf.number);
                    // }else{//
                    //     ret=enqueue_from_user(&(context->read_fifo),context->read_check,true,recover_buf.buf+2,recover_buf.size-3,recover_buf.number);
                    // }
                    
                }
            }
            //缓存该帧
            if(is_head){
                ret=replace_queue_from_user(&(context->read_fifo),context->read_check,true,window_id,recover_buf.buf+3,recover_buf.size-4,recover_buf.number);
            }else{//
                ret=replace_queue_from_user(&(context->read_fifo),context->read_check,true,window_id,recover_buf.buf+2,recover_buf.size-3,recover_buf.number);
            }
            //将该数据包放入
        }
        if(ret>=0)write_respond(context,recover_buf.number,ACK);//装入缓冲区成功，反馈发送者
        if((flag&LF)==LF && ret >= 0){//rear帧成功放入缓存内
            is_rear=1;
            rf_number=recover_buf.number;//记录最后帧的序号，以便后面发送确认
        }
        if(is_rear && check_read_window(context)){//所有包已经接收完成
            printf("all read done\n");
            write_respond(context,rf_number,ACK|RF);//装入缓冲区成功，反馈发送者
            time_out=context->retry_timeout_us * 2;//修改超时等待反馈
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
