"""Compile the production, bounded visual fallback with real conversation examples."""
import pathlib
import subprocess
import tempfile
import unittest

class ConversationFace(unittest.TestCase):
    def test_visual_hints_and_negation(self):
        header=pathlib.Path(__file__).resolve().parents[2]/'main/boards/waveshare/esp32-s3-touch-amoled-1.75/conversation_face.h'
        code=r'''
#include "HEADER"
#include <cassert>
int main(){using namespace conversation_face;
assert(Hint("Hoje estou triste",true)=="worried");
assert(Hint("Tenho medo",true)=="worried");
assert(Hint("Estou feliz com o resultado",true)=="happy");
assert(Hint("Parabéns pela conquista!",false)=="happy");
assert(Hint("Sinto muito por isso",false)=="worried");
assert(Hint("Vamos resolver passo a passo",false)=="focused");
assert(Hint("Qual é a capital de Portugal?",true).empty());
assert(Hint("Não estou triste",true).empty());
assert(Hint("nao estou feliz",true).empty());
assert(Hint(std::string(1200,'x')+" estou triste",true).empty());
assert(Neutral("robot_2") && Neutral("normal") && !Neutral("happy"));
}
'''.replace('HEADER',str(header))
        with tempfile.TemporaryDirectory() as folder:
            p=pathlib.Path(folder);(p/'test.cpp').write_text(code)
            subprocess.run(['c++','-std=c++17',str(p/'test.cpp'),'-o',str(p/'test')],check=True,capture_output=True)
            subprocess.run([str(p/'test')],check=True,capture_output=True)
