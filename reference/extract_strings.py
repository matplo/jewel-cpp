"""Extract unchanged MAKESTRINGS_VAC/CLEANUP/ISDIQUARK from pinned stock source."""
import hashlib,json,re,sys
from pathlib import Path
source,output=map(Path,sys.argv[1:])
manifest=json.loads(Path(__file__).with_name('medium-source.json').read_text())
p=source/'jewel-2.6.0.f.stock'
if not p.exists():p=source/'jewel-2.6.0.f'
data=p.read_bytes()
if hashlib.sha256(data).hexdigest()!=manifest['jewel-2.6.0.f.stock']['sha256']:raise SystemExit('Unrecognized source')
lines=data.decode().splitlines(keepends=True);blocks=[]
for name in ['MAKESTRINGS_VAC','CLEANUP','ISDIQUARK']:
 start=next(i for i,line in enumerate(lines) if re.match(r'\s*(?:logical function|subroutine) '+name+r'(?:\(|\s*$)',line,re.I))
 end=next(i for i in range(start+1,len(lines)) if re.fullmatch(r'\s*end\s*',lines[i],re.I))
 blocks.append(''.join(lines[start:end+1]))
output.write_text(''.join(blocks))
