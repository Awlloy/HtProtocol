#ifndef VAL_DATA_H
#define VAL_DATA_H
#include <stdio.h>
#include <string.h>



#ifndef UINT_TYPE
#define UINT_TYPE
typedef int int32_th;
typedef unsigned char uint8_th;
typedef char int8_th;
typedef unsigned short uint16_th;
typedef unsigned int uint32_th;
typedef long long int64_th;
typedef unsigned long long uint64_th;
#endif

/*
全加和校验
*/
uint8_th get_val_sum(uint8_th org_val,uint8_th *send_data,int size);
/*
异或和校验
*/
uint8_th get_val_xor_sum(uint8_th org_val,uint8_th *send_data,int size);

#endif

