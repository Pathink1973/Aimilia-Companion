"""Exercise production counters with silence, speech and concurrent updates."""
import subprocess
import tempfile
import unittest
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
class PlaybackCounters(unittest.TestCase):
    def test_counter_snapshot(self):
        with tempfile.TemporaryDirectory() as tmp:
            source=Path(tmp)/'test.cc';binary=Path(tmp)/'test'
            source.write_text('#include "'+str(ROOT/'main/audio/playback_diagnostics.h')+'"\n'+r'''
#include <cassert>
#include <thread>
int main(){
 PlaybackDiagnostics d; int16_t silence[16]={}, speech[16]={-32768};
 d.Played(silence,16);d.Played(speech,16);d.Played(nullptr,0);
 auto c=d.Get();assert(c.played==3 && c.audible==1 && c.decoded==0);
 auto run=[&](){for(int i=0;i<10000;i++){d.Decoded();d.Played(speech,16);}};
 std::thread a(run),b(run);a.join();b.join();c=d.Get();
 assert(c.decoded==20000 && c.played==20003 && c.audible==20001);
}
''')
            subprocess.run(['c++','-std=c++17','-pthread',str(source),'-o',str(binary)],check=True)
            subprocess.run([str(binary)],check=True)
