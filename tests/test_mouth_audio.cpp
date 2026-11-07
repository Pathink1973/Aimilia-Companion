#include "mouth_audio.h"
#include <cassert>
int main() {
 int16_t quiet[8]={}; int16_t loud[8]={-32768,0,0,0,32767,0,0,0};
 assert(mouth_audio::Measure(quiet,8)==0);
 assert(mouth_audio::Measure(loud,8)==100);
 assert(mouth_audio::Measure(nullptr,0)==0);
 mouth_audio::updated_ms=0xfffffff0; mouth_audio::level=70;
 assert(mouth_audio::Read(20)==70); assert(mouth_audio::Read(500)==0);
}
