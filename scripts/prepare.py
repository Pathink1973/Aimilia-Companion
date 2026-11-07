#!/usr/bin/env python3
"""Prepare a pinned, isolated XiaoZhi worktree. Never flashes or reads a device."""
import argparse, hashlib, json, shutil, subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--upstream-local',type=Path,help='Optional local Git mirror; only read, never modified')
    args=parser.parse_args()
    manifest=json.loads((ROOT/'firmware/manifest.json').read_text())
    dest=ROOT/'work/xiaozhi'
    if dest.exists():raise SystemExit('work/xiaozhi already exists. Preserve your work; choose a fresh clone to repeat preparation.')
    for name,digest in manifest['overlay'].items():
        if hashlib.sha256((ROOT/'firmware/overlay'/name).read_bytes()).hexdigest()!=digest:
            raise SystemExit('Overlay checksum mismatch: '+name)
    dest.parent.mkdir(exist_ok=True)
    source=str(args.upstream_local.resolve()) if args.upstream_local else manifest['upstream']
    subprocess.run(['git','clone','--no-checkout',source,str(dest)],check=True)
    subprocess.run(['git','checkout','--detach',manifest['revision']],cwd=dest,check=True)
    head=subprocess.check_output(['git','rev-parse','HEAD'],cwd=dest,text=True).strip()
    if head!=manifest['revision']:raise SystemExit('Unexpected upstream revision')
    for name in manifest['overlay']:
        target=dest/name;target.parent.mkdir(parents=True,exist_ok=True)
        shutil.copy2(ROOT/'firmware/overlay'/name,target)
    board=dest/'main/boards/waveshare'/manifest['board']
    shutil.copy2(ROOT/'firmware/profiles/aimilia.config.json',board/'aimilia.config.json')
    shutil.copy2(ROOT/'firmware/dependencies.lock',dest/'dependencies.lock')
    print('Prepared:',dest)
    print('No device was accessed. Next: source ESP-IDF 6.0.1 export.sh, then python3 scripts/build.py')
if __name__=='__main__':main()
