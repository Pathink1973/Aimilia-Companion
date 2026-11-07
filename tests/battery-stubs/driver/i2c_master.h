#pragma once
#include <cstdint>
#include <cstddef>
using i2c_master_dev_handle_t=void*;
constexpr int ESP_OK=0;
extern int fail_at, calls;
extern uint8_t status_value, percent_value;
inline int i2c_master_transmit_receive(void*,const uint8_t* reg,size_t,uint8_t* result,size_t,int) {
 ++calls;
 if(calls==fail_at)return -1;
 *result=*reg==1 ? status_value : percent_value;
 return ESP_OK;
}
