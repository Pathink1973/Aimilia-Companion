#!/usr/bin/env python3
"""Host tests of the exported code, with no board and no network access."""
import subprocess, tempfile, unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
overlay=ROOT/'firmware/overlay'
board=overlay/'main/boards/waveshare/esp32-s3-touch-amoled-1.75'
suite=unittest.TestLoader().discover(str(overlay/'scripts/tests'),pattern='test_*.py')
result=unittest.TextTestRunner(verbosity=2).run(suite)
if not result.wasSuccessful():raise SystemExit(1)
for name in ['battery','formas','kubi_animation','mouth_audio']:
    with tempfile.TemporaryDirectory() as tmp:
        binary=Path(tmp)/'test'
        subprocess.run(['c++','-std=c++17','-I'+str(board),'-I'+str(ROOT/'tests/battery-stubs'),str(ROOT/f'tests/test_{name}.cpp'),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True)
    print('PASS host:',name)
