"""Extract the unedited backward timing block, with explicit harness stubs."""
import hashlib,json,sys
from pathlib import Path
source,output=map(Path,sys.argv[1:]);path=source/'jewel-2.6.0.f.stock'
data=path.read_bytes();manifest=json.loads(Path(__file__).with_name('medium-source.json').read_text())
if hashlib.sha256(data).hexdigest()!=manifest[path.name]['sha256']:raise SystemExit('Unrecognized source')
text=data.decode();start=text.index('C--walk backwards from ME to beam and set production vertices and times')
end=text.index('C--order all singlets',start)
header="""      subroutine isrtimes
      implicit none
      integer n,k(23000,5),i,lstart,lprev,l1,l2,jj
      double precision p(23000,5),mv(23000,5),masses(23000)
      double precision ftfac,x0,y0,ltime,lambda,pyp,pymass,pyr
      logical isrscat
      common /input/ n,k,p,mv,masses,ftfac,x0,y0,ltime,isrscat
"""
output.write_text(header+text[start:end]+'      end\n')
