#!/usr/bin/env python3
"""Freeze explicitly supplied reference inputs; never mix stock/custom/perf labels."""
import argparse, hashlib, json, platform, shutil, subprocess
from pathlib import Path
p=argparse.ArgumentParser()
p.add_argument('--variant',choices=['stock','custom','perf'],required=True)
p.add_argument('--output',type=Path,required=True)
p.add_argument('--file',action='append',type=Path,required=True,
               help='Repeat for binaries, cards, PDF data, patches and source files')
a=p.parse_args()
if a.output.exists(): p.error('output already exists; choose a new immutable baseline directory')
files=[]
for f in a.file:
    if not f.is_file(): p.error(f'not a file: {f}')
    files.append((f,f.read_bytes()))
a.output.mkdir(parents=True)
manifest={'variant':a.variant,'platform':platform.platform(),'files':[]}
for i,(f,data) in enumerate(files):
    target=a.output/f'{i:04d}-{f.name}'
    shutil.copy2(f,target)
    manifest['files'].append({'original':str(f.resolve()),'snapshot':target.name,
        'sha256':hashlib.sha256(data).hexdigest(),'bytes':len(data)})
(a.output/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
print(a.output/'manifest.json')
