
#include "val_data.h"
/*
全加和校验
*/
uint8_th get_val_sum(uint8_th org_val,uint8_th *send_data,int size){
    for(int i=0;i<size;++i){
        // sum+=(uint8_th)(send_data[i]);
        org_val+=(uint8_th)(send_data[i]);
    }
    return org_val;
}
/*
异或和校验
*/
uint8_th get_val_xor_sum(uint8_th org_val,uint8_th *send_data,int size){
    for(int i=0;i<size;++i){
        // sum+=(uint8_th)(send_data[i]);
        org_val^=(uint8_th)(send_data[i]);
    }
    return org_val;
}


