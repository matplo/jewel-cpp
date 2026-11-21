import hashlib,json,re,sys
from pathlib import Path
source,output=map(Path,sys.argv[1:]);p=source/'jewel-2.6.0.f.stock'
if not p.exists():p=source/'jewel-2.6.0.f'
data=p.read_bytes();manifest=json.loads(Path(__file__).with_name('medium-source.json').read_text())
if hashlib.sha256(data).hexdigest()!=manifest['jewel-2.6.0.f.stock']['sha256']:raise SystemExit('Unrecognized source')
lines=data.decode().splitlines(keepends=True)
start=next(i for i,x in enumerate(lines) if re.match(r'\s*subroutine putparticlesonshell\(',x,re.I))
end=next(i for i in range(start+1,len(lines)) if re.fullmatch(r'\s*end\s*',lines[i],re.I))
output.write_text(''.join(lines[start:end+1]))
