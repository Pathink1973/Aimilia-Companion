"""Exercise production superellipse math independently of the ESP/LVGL runtime."""
import pathlib
import subprocess
import tempfile
import unittest


class FaceGeometry(unittest.TestCase):
    def test_bounds_area_and_settling(self):
        header = pathlib.Path(__file__).resolve().parents[2] / 'main/boards/waveshare/esp32-s3-touch-amoled-1.75/face_geometry.h'
        code = r'''
#include "HEADER"
#include <cassert>
int main() {
    for (int frame=0; frame<64; ++frame) {
        float stretch=(1+.012f*std::sin(frame*.09f))*1.025f;
        for (int i=0;i<=96;++i) {
            auto p=aimilia_face::Superellipse(i*6.2831853f/96,1,1);
            assert(std::abs(std::pow(std::abs(p.x),2.7f)+std::pow(std::abs(p.y),2.7f)-1)<.00001f);
            float x=140+p.x*132*stretch, y=130+p.y*119/stretch;
            assert(x>=1 && x<=279 && y>=1 && y<=259);
        }
        assert(std::abs((132*stretch)*(119/stretch)-132*119)<.01f);
    }
    aimilia_face::Spring spring;
    for(int i=0;i<200;i++){spring.Step(1,.05f);assert(std::isfinite(spring.value));assert(spring.value>=0 && spring.value<1.2f);}
    assert(std::abs(spring.value-1)<.0001f);
}
'''.replace('HEADER',str(header))
        with tempfile.TemporaryDirectory() as folder:
            src=pathlib.Path(folder)/'geometry.cpp'; binary=pathlib.Path(folder)/'geometry'
            src.write_text(code)
            subprocess.run(['c++','-std=c++17',str(src),'-o',str(binary)],check=True,capture_output=True)
            subprocess.run([str(binary)],check=True,capture_output=True)
