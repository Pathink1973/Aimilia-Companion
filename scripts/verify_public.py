#!/usr/bin/env python3
"""Offline checks: local links, allowed public files, overlay integrity and secret patterns.
This is a focused check, not a guarantee that every kind of secret is detectable.
"""
from pathlib import Path
import hashlib, json, re, sys
from urllib.parse import unquote
ROOT=Path(__file__).resolve().parents[1]
SKIP={'.git','work','node_modules','.wrangler','__pycache__','dist'}
files=[p for p in ROOT.rglob('*') if p.is_file() and not SKIP.intersection(p.relative_to(ROOT).parts)]
errors=[]
patterns=[
    ('private key',r'-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----'),
    ('Google OAuth secret',r'GOCSPX-[A-Za-z0-9_-]{12,}'),
    ('Google refresh token',r'1//[A-Za-z0-9_-]{30,}'),
    ('GitHub token',r'gh[pousr]_[A-Za-z0-9]{24,}'),
    ('realistic MCP token',r'wss://api\.xiaozhi\.me/mcp/\?token=[A-Za-z0-9._-]{40,}'),
    ('private local path',r'/' + r'Users/[^/\s]+/'),
]
for p in files:
    rel=p.relative_to(ROOT)
    if any(x in {'.secrets','.env','.dev.vars','backups','build','build-dual'} for x in rel.parts):errors.append(f'Forbidden private path: {rel}')
    if p.suffix in {'.bin','.elf','.pem','.key','.log'}:errors.append(f'Forbidden artifact: {rel}')
    try:s=p.read_text()
    except UnicodeDecodeError:continue
    for label,pattern in patterns:
        if re.search(pattern,s):errors.append(f'{label}: {rel} (value withheld)')
    if p.suffix=='.md':
        for url in re.findall(r'\]\(([^\s)]+)(?:\s+"[^"]*")?\)',s):
            if url.startswith(('http:','https:','mailto:','#')):continue
            target=(p.parent/unquote(url.split('#')[0])).resolve()
            if not target.is_relative_to(ROOT) or not target.exists():errors.append(f'Broken local link: {rel} -> {url}')
m=json.loads((ROOT/'firmware/manifest.json').read_text())
for name,digest in m['overlay'].items():
    p=ROOT/'firmware/overlay'/name
    if not p.exists() or hashlib.sha256(p.read_bytes()).hexdigest()!=digest:errors.append('Overlay hash mismatch: '+name)
profile=json.loads((ROOT/'firmware/profiles/aimilia.config.json').read_text())
assert profile['target']=='esp32s3'
assert profile['builds'][0]['name']=='esp32-s3-touch-amoled-1.75c'
assert 'CONFIG_ESPTOOLPY_FLASHSIZE_32MB=y' in profile['builds'][0]['sdkconfig_append']
assert len(re.findall(r'\{"[a-z_]+", \{', (ROOT/'firmware/overlay/main/boards/waveshare/esp32-s3-touch-amoled-1.75/expressive_eyes.h').read_text(),re.M))==18
if errors:
    print('\n'.join(errors));sys.exit(1)
print(f'PASS: {len(files)} public files, local documentation links, 18 expressions, profile and {len(m["overlay"])} overlay hashes.')
