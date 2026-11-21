"""Extract an unchanged one-transfer section; no colour/storage/outer retry claims."""
import hashlib,json,sys
from pathlib import Path
source,output=map(Path,sys.argv[1:])
manifest=json.loads(Path(__file__).with_name('collision-source.json').read_text())
p=source/manifest['file']
if not p.exists():p=source/'jewel-2.6.0.f'
data=p.read_bytes()
if hashlib.sha256(data).hexdigest()!=manifest['sha256']:raise SystemExit('Unknown JEWEL source')
lines=data.decode().splitlines(keepends=True)
def section(name):
 a,b=manifest[name]['lines'];s=''.join(lines[a-1:b])
 if hashlib.sha256(s.encode()).hexdigest()!=manifest[name]['sha256']:raise SystemExit('Wrong section')
 return s
header='''      subroutine pair_reference(L,N1,N2,NEWM,DMLEFT,TTOT,
     & DELTAM,DM,DIR,SPLITREC,Z4,QQBARDEC,REJECTT,RESHUFFLE,NEWM2)
'''
setup='''      line=l
      j=n1
      lpart=l
      isrecoil=k(l,1).eq.2
'''
output.write_text(header+section('declarations')+setup+section('body')+' 222  continue\n      end\n')
