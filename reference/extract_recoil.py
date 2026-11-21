"""Wrap the unchanged DOKINEMATICS recoil-storage section for isolated replay."""
import hashlib,json,sys
from pathlib import Path
source,output=map(Path,sys.argv[1:])
manifest=json.loads(Path(__file__).with_name('recoil-source.json').read_text())
p=source/manifest['file']
if not p.exists():p=source/'jewel-2.6.0.f'
data=p.read_bytes()
if hashlib.sha256(data).hexdigest()!=manifest['sha256']:raise SystemExit('Unrecognized JEWEL source')
start,end=manifest['lines']
block=''.join(data.decode().splitlines(keepends=True)[start-1:end])
if hashlib.sha256(block.encode()).hexdigest()!=manifest['section_sha256']:raise SystemExit('Recoil section mismatch')
header='''      subroutine recoil_reference(p,k,n,scatcen,dummies,scatflav,
     & nscatcen,maxnscatcen,recmode,writescatcen,dosubtraction,
     & rejectt,softrec)
      implicit none
      integer n,k(3,5),scatflav(23000)
      integer nscatcen,maxnscatcen,recmode,logfid
      double precision p(3,5),scatcen(23000,5),dummies(23000,5)
      double precision qmass2,pdummy
      logical writescatcen,dosubtraction,rejectt,softrec
      logfid=0
      pdummy=1.d-6
'''
output.write_text(header+block+'      end\n')
