#include <cassert>
#include <cstdio>
#include <vector>
#include "formas_renderer.h"
using namespace aimilia_formas;
int main(int argc,char**argv){
 std::vector<uint32_t> base(kPixels+2,0x12345678),frame(kPixels+2,0x12345678);
 for(int shape=0;shape<6;++shape){Raster b(base.data()+1);b.Body(shape,kColors[shape]);
  assert(base.front()==0x12345678&&base.back()==0x12345678);int opaque=0;for(size_t i=1;i<=kPixels;++i)opaque+=(base[i]>>24)==255;assert(opaque>30000&&opaque<65000);
  for(int e=0;e<18;++e){assert(Emotion(kNames[e])==e);for(bool blink:{false,true}){std::memcpy(frame.data()+1,base.data()+1,kPixels*4);Raster r(frame.data()+1);r.base_=base.data()+1;r.Draw(e,blink,3,blink?0:80,12345,false);assert(frame.front()==0x12345678&&frame.back()==0x12345678);}}
 }
 if(argc>1){Raster b(base.data()+1);b.Body(5,kColors[5]);std::memcpy(frame.data()+1,base.data()+1,kPixels*4);Raster r(frame.data()+1);r.base_=base.data()+1;r.Draw(17,false,0,0,12345,false);FILE*f=fopen(argv[1],"wb");assert(f);fprintf(f,"P6\n320 320\n255\n");for(size_t i=1;i<=kPixels;++i){auto p=frame[i];if(!(p>>24))p=0xF4EDDF;unsigned char rgb[]={static_cast<unsigned char>(p>>16),static_cast<unsigned char>(p>>8),static_cast<unsigned char>(p)};fwrite(rgb,1,3,f);}fclose(f);}
 puts("216 shape / expression / blink cases pass; buffer guards intact; all 18 mappings pass");
}
