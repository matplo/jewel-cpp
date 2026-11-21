import hashlib,json,sys,re
from pathlib import Path
source,output=map(Path,sys.argv[1:])
manifest=json.loads(Path(__file__).with_name('kinematics-source.json').read_text())
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
        declarations=sum(bool(re.match(r'\s*(?:(?:(?:DOUBLE PRECISION|LOGICAL|INTEGER) )?FUNCTION|SUBROUTINE)\s+',line,re.I)) for line in unit)
        if declarations!=1: raise SystemExit('Reference span includes multiple routines: '+name)
        if not re.fullmatch(r'\s*(?:\d+\s*)?END\s*',unit[-1],re.I): raise SystemExit('Reference span misses END: '+name)
        blocks.extend(unit)
    for name,(start,end) in item.get('blocks',{}).items():
        if name!='GENEVENT status preparation':raise SystemExit('Unknown block')
        unit=lines[start-1:end]
        if 'K(LME1,1)=1' not in unit[0] or 'CONTINUE' not in unit[-1]:raise SystemExit('Preparation span mismatch')
        blocks.append("""
      SUBROUTINE PREPAREPARTONS(LME1,LME2,ETAMAX)
      IMPLICIT NONE
      INTEGER LME1,LME2,N,NPAD,K,IPART,OFFSET,NPART,PID
      DOUBLE PRECISION P,V,ETAMAX,PYMASS,PYP
      LOGICAL ISQORG
      COMMON/PYJETS/N,NPAD,K(23000,5),P(23000,5),V(23000,5)
      OFFSET=0
      NPART=N
""")
        blocks.extend(unit)
        blocks.append('      END\n')
output.write_text(''.join(blocks))
