
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
    //检查是否在可接收窗口内
    if(  head==rear //窗口内无数据
    || offest < WINDOW_SIZE //在可缓存的下标范围内,无需
    ){
        return offest;
    }
    return -1;
}
int read_pack_to_window(HtProtocolContext *context,int window_id,HtBuffer *recover_buf,int buf_offest,int size_offest,int update){//将读取到的数据装入到读取数据窗口
    HtBuffer *window_buf;
    int window_id;
    int i;
    int head=context->read_fifo.head;
    int rear=context->read_fifo.rear;
    int offest;
    int head_number=context->read_fifo.fifo[head].number;
    context->read_fifo.fifo[i]
    // context->read_check[context->read_fifo.head]=false;//头帧标记为未收到
    // offest = recover_buf->number - head_number;
    // if(offest<0)offest+=NUMBER_MAX_SIZE;
    // //检查是否在可接收窗口内
    // if(  head==rear //窗口内无数据
    // || offest < WINDOW_SIZE //在可缓存的下标范围内,无需
    // ){
    //     window_id = (context->read_fifo.head+offest)%WINDOW_SIZE;//新帧在窗口的坐标
    //     if(context->read_check[window_id] || update){//未缓存该帧
    //         return 1;//已缓存过
    //     }
    //     // if(offest == WINDOW_SIZE - 1 ){//最后一个下标,需要将head移动.即替换head,过滤最旧数据
    //     if(offest == WINDOW_SIZE - 1){//最后一个下标,当head已经接收过时,需要将head移动.即替换head,过滤最旧已经接收到的数据数据
    //         if(!context->read_check[head]){//fifo head包未收到,无法替换
    //             return -1;
    //         }
    //         //即将要去掉head包,将数据拷贝到用户数据中
    //         context->read_fifo.head=(context->read_fifo.head+1)%WINDOW_SIZE;

    //     }
    //     memcpy(context->read_fifo.fifo[window_id].buf,recover_buf->buf+buf_offest,recover_buf->size+size_offest);
    //     context->read_fifo.fifo[window_id].size=recover_buf->size+size_offest;
    //     context->read_fifo.fifo[window_id].number=recover_buf->number;
    //     context->read_check[window_id]=true;//更新为已缓存
    //     return 0;//缓存成功
    // }
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
        //缓存数据包
        window_id_offest=get_pack_to_window_offest(context,recover_buf.number);
        if(window_id_offest!=-1){//计算新包的number在fifo中与head的偏移量,-1为无法缓存
            window_id = (context->read_fifo.head+window_id_offest)%WINDOW_SIZE;//新帧在窗口的坐标
            if(!context->read_check[window_id]){//未缓存该帧
                if(window_id_offest == WINDOW_SIZE - 1){//最后一个下标,当head已经接收过时,需要将head移动.即替换head,过滤最旧已经接收到的数据数据
                    if(context->read_check[context->read_fifo.head]){//fifo head包已收到,可以替换掉head
                        //并且将head包拷贝到用户数据
                        // buf_ptr=&(context->read_fifo.fifo[context->read_fifo.head]);
                        // if(cp_pack_idx+buf_ptr->size > size)return -1;//缓冲不足
                        // memcpy(user_buf+cp_pack_idx,buf_ptr->buf,buf_ptr->size);
                        // cp_pack_idx+=cp_size;
                        // //head后移
                        // context->read_fifo.head=(context->read_fifo.head+1)%WINDOW_SIZE;
                        int ic=context->read_fifo.head;
                        int rear=context->read_fifo.rear;
                        buf_ptr=&(context->read_fifo.fifo[ic]);
                        if(cp_pack_idx+buf_ptr->size > size)return -1;//缓冲不足
                        //拷贝head包
                        memcpy(user_buf+cp_pack_idx,buf_ptr->buf,buf_ptr->size);
                        cp_pack_idx+=buf_ptr->size;
                        context->cp_pack_number++;
                        context->read_check[ic]=false;//将该窗口接收标志修改为false,以便后面的包复用

                        //将该数据包放入
                        if(is_head){
                            ret=read_pack_to_window(context,window_id,&recover_buf,3,-4,0);//尝试将数据放入接收缓冲
                        }else{//
                            ret=read_pack_to_window(context,window_id,&recover_buf,2,-3,0);//尝试将数据放入接收缓冲
                        }

                        for(;ic!=rear;ic=(ic+1)%WINDOW_SIZE){
                            if(context->read_fifo.fifo[ic].number == context->cp_pack_number
                                && context->read_check[ic]){//已经接收过，可以写入
                                buf_ptr=&(context->read_fifo.fifo[ic]);
                                if(cp_pack_idx+buf_ptr->size > size)return -1;//缓冲不足
                                memcpy(user_buf+cp_pack_idx,buf_ptr->buf,buf_ptr->size);
                                cp_pack_idx+=buf_ptr->size;
                                context->cp_pack_number++;
                                context->read_check[ic]=false;//将该窗口接收标志修改为false,以便后面的包复用
                                continue;
                            }
                            break;
                        }
                        context->read_fifo.head=ic;//修改处理后的结果
                    }
                }
                
            }
        }
        
        // if(recover_buf.number != context->cp_pack_number){//出现顺序错误包
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
