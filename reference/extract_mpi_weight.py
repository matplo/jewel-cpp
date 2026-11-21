"""Extract unchanged bin/weight expressions from frozen PYMULT."""
from pathlib import Path
import hashlib,sys
source,output=map(Path,sys.argv[1:])
raw=source.read_bytes()
if hashlib.sha256(raw).hexdigest()!='9fcfed664e4ac34462aa63679f2dd3e935c9f6f63f7cd6a785b73ccb82740ed9':raise SystemExit('Unrecognized Pythia6 source')
s=raw.decode();s=s[s.index('      SUBROUTINE PYMULT(MMUL)'):]
def extract(start,end):return s[s.index(start):s.index(end,s.index(start))+len(end)]
bin_code=extract('        RBIN=MAX(0.000001D0','        IRBIN=INT(1D0+20D0*RBIN)')
weight=extract('        VINT(150)=1D0 ', '     &    SIGABV/MAX(1D-10,SIGT(0,0,5))))\n        ENDIF')
output.write_text("""      PROGRAM PROBE
      IMPLICIT DOUBLE PRECISION(A-H,O-Z)
      DIMENSION VINT(400),MINT(400),MSTP(200),SIGT(0:6,0:6,0:5)
      DIMENSION NMUL(20),SIGM(20)
   10 READ(*,*,END=20) XTS,VINT(149),VINT(148),SIGT(0,0,5),
     & VNT146,VINT(2),MINT(39)
      READ(*,*) SIGM
      READ(*,*) NMUL
      VINT(289)=1D0
      MSTP(171)=1
      SIGABV=0D0
"""+bin_code+'\n'+weight+"""
      WRITE(*,'(4ES25.17)') RBIN,DBLE(IRBIN),SIGABV,VINT(150)
      GOTO 10
   20 END
""")
