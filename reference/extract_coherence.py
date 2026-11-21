"""Hash-checked unmodified DOCOHSCATTERING sections; standalone test wrapper."""
import hashlib,json,sys
from pathlib import Path
source,output=map(Path,sys.argv[1:])
m=json.loads(Path(__file__).with_name('coherence-source.json').read_text())
p=source/m['file']
if not p.exists():p=source/'jewel-2.6.0.f'
data=p.read_bytes()
if hashlib.sha256(data).hexdigest()!=m['sha256']:raise SystemExit('Unknown JEWEL source')
lines=data.decode().splitlines(keepends=True);blocks={}
for name,s in m['sections'].items():
 a,b=s['lines'];block=''.join(lines[a-1:b])
 if hashlib.sha256(block.encode()).hexdigest()!=s['sha256']:raise SystemExit('Section mismatch')
 blocks[name]=block
header='''      subroutine coherent_reference(mode,cohlengthfac,splitangle,
     & tnow,splittime,allqs,z,phisplit,qperp,dperp,pz,pts,resolved)
      implicit none
      integer mode,n,npad,k(23000,5),l,nend,sib1,sib2
      double precision p(23000,5),v(23000,5),allqs(1,6)
      common /PYJETS/ n,npad,k,p,v
      double precision cohlengthfac,splitangle,tnow,splittime,z
      double precision phisplit,qperp,dperp,pz,pts,qdotp,q2,pyp
      double precision thetam,phim,mb,mc
      logical resolved
      l=3
      nend=1
      sib1=1
      sib2=2
      resolved=.false.
      if(mode.eq.1)then
'''
# The gate block ends with the original IF; only its observable body is wrapped.
output.write_text(header+blocks['gate']+'''        resolved=.true.
      endif
      else
'''+blocks['axial']+'''      if(pts.lt.0.d0)return
'''+blocks['daughters']+'''      endif
      end
''')
start_header='''      subroutine coherent_start_reference(l,cohscat,tstart,tsum,
     & splitangle,mv,trip,anti,zd,thetad,qqbard,didcohscat,deltat,
     & tleft,z,phisplit,quark,qqbar,dir)
      implicit none
      integer n,npad,k(23000,5),l,trip(23000),anti(23000)
      common /PYJETS/ n,npad,k,p,v
      double precision p(23000,5),v(23000,5),mv(23000,5)
      double precision zd(23000),thetad(23000)
      logical qqbard(23000),cohscat,didcohscat,quark,qqbar
      double precision tstart,tsum,splitangle,deltat,tleft,z
      double precision phisplit,tnow,decaytl,decaytsib,decayt1,decayt2
      double precision mom1(4),mom2(4),pos1(4),pos2(4),pyp
      double precision thetam,phim
      integer count2,nscatl,mother,sib,sib1,sib2,dir,logfid
      integer nbef,lbef,lsib,klbef1,klsib1,i,lcohorig
      logfid=0
'''
with output.open('a') as f:f.write(start_header+blocks['start']+'      end\n')
