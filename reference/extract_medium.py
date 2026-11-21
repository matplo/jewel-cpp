import hashlib,json,sys,re
from pathlib import Path
source,output=map(Path,sys.argv[1:3])
vacuum=len(sys.argv)==4 and sys.argv[3]=='--vacuum'
if len(sys.argv)>3 and not vacuum:raise SystemExit('unknown extraction mode')
vacuum_units={}
if vacuum:
    data=(source/'medium-vac.f.stock').read_bytes()
    if hashlib.sha256(data).hexdigest()!='c83fda6a0b81ce0dc9f524bbf9a3c68f0225f090107a5cd4d6711edad0b20ea0':raise SystemExit('Unrecognized vacuum source')
    lines=data.decode().splitlines(keepends=True)
    for i,line in enumerate(lines):
        match=re.match(r'\s*(?:DOUBLE PRECISION FUNCTION|SUBROUTINE)\s+(\w+)',line,re.I)
        if not match:continue
        end=next(j for j in range(i+1,len(lines)) if re.fullmatch(r'\s*END\s*',lines[j],re.I))
        vacuum_units[match[1].upper()]=''.join(lines[i:end+1])
manifest=json.loads(Path(__file__).with_name('medium-source.json').read_text())
blocks=[]
for filename,item in manifest.items():
    path=source/filename
    if not path.exists(): path=source/filename.removesuffix('.stock')
    data=path.read_bytes()
    if hashlib.sha256(data).hexdigest()!=item['sha256']: raise SystemExit('Refusing unrecognized source: '+filename)
    lines=data.decode().splitlines(keepends=True)
    for name,(start,end) in item['routines'].items():
        if not re.search(r'(FUNCTION|SUBROUTINE)\s+'+name+r'(\(|\s*$)',lines[start-1],re.I): raise SystemExit('Routine mismatch')
        unit=lines[start-1:end]
        declarations=sum(bool(re.match(r'\s*(?:(?:DOUBLE PRECISION|LOGICAL) FUNCTION|SUBROUTINE)\s+',line,re.I)) for line in unit)
        if declarations!=1: raise SystemExit('Reference span includes multiple routines: '+name)
        if not re.fullmatch(r'\s*(?:\d+\s*)?END\s*',unit[-1],re.I): raise SystemExit('Reference span misses END: '+name)
        text=''.join(unit)
        for change in item.get('declaration_normalizations',{}).get(name,[]):
            if text.count(change['from'])!=1: raise SystemExit('Declaration normalization mismatch: '+name)
            text=text.replace(change['from'],change['to'])
        if filename=='medium-simple.f.stock' and name.upper() in vacuum_units:text=vacuum_units[name.upper()]
        blocks.append(text)
output.write_text(''.join(blocks))
