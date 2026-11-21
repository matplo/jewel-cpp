import hashlib,json,sys,re
from pathlib import Path
source,output=map(Path,sys.argv[1:])
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
        blocks.append(text)
output.write_text(''.join(blocks))
