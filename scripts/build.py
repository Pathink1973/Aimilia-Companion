#!/usr/bin/env python3
"""Compile the standalone public profile. Never flash a connected board."""
import json, os, shutil, subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
m=json.loads((ROOT/'firmware/manifest.json').read_text())
work=ROOT/'work/xiaozhi'
if not work.exists():raise SystemExit('Run python3 scripts/prepare.py first.')
if not os.environ.get('IDF_PATH') or not shutil.which('idf.py'):
    raise SystemExit('Activate ESP-IDF 6.0.1 in this shell first: source /path/to/esp-idf/export.sh')
subprocess.run(['idf.py','--version'],check=True)
subprocess.run(['python3','scripts/build.py','waveshare/'+m['board'],'--config','aimilia.config.json','--name',m['variant'],'--language',m['locale'],'--wake-word','disabled'],cwd=work,check=True)
print('Compiled only. Review docs/03-instalacao.md before writing any device.')
