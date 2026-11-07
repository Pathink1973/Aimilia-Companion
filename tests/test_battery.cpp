#include <initializer_list>
#include <cassert>
#include "battery_status.h"
int fail_at=0,calls=0;uint8_t status_value=0,percent_value=0;
int main(){
 int level;bool charge,discharge;
 for(int fail:{1,2}) {
  calls=0;fail_at=fail;level=42;charge=true;discharge=true;
  assert(!ReadWaveshareBatteryStatus(nullptr,level,charge,discharge));
  assert(level==42 && charge && discharge);
 }
 fail_at=0;
 for(int direction=0;direction<4;++direction) {
  calls=0;status_value=direction<<5;percent_value=85;
  assert(ReadWaveshareBatteryStatus(nullptr,level,charge,discharge));
  assert(level==85 && charge==(direction==1) && discharge==(direction==2));
 }
 calls=0;percent_value=255;level=42;
 assert(!ReadWaveshareBatteryStatus(nullptr,level,charge,discharge));assert(level==42);
}
