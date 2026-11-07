#include "kubi_animation.h"
#include <cassert>
int main(){
 kubi_animation::Sequence s;
 assert(s.Update(60001,true,false,false,false).sleep);
 s.React(60100,false);
 assert(s.Update(60200,true,false,false,false).emotion=="surprised");
 assert(s.Update(61000,true,false,false,false).sparkle);
 assert(s.Update(61100,false,false,false,false).emotion.empty());
 assert(!s.Update(62000,true,false,false,false).sparkle);
 s.React(63000,true);
 assert(s.Update(63100,true,false,false,false).emotion=="surprised");
 assert(s.Update(64000,true,false,false,false).emotion=="angry");
 assert(s.Update(66500,true,false,false,false).emotion=="normal");
 assert(s.Update(67000,true,false,false,false).emotion.empty());
 s.React(68000,false);
 assert(s.Update(68100,true,true,false,false).emotion.empty());
 s.React(69000,false);
 assert(s.Update(69100,true,false,true,false).emotion.empty());
 s.Update(0xffffff00,false,false,false,false);
 assert(!s.Update(100,true,false,false,false).sleep);
}
