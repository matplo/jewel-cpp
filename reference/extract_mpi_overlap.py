"""Extract PYMULT initialization and hard-impact sampling unchanged."""
from pathlib import Path
import hashlib,sys
source,output=map(Path,sys.argv[1:]);raw=source.read_bytes()
if hashlib.sha256(raw).hexdigest()!='9fcfed664e4ac34462aa63679f2dd3e935c9f6f63f7cd6a785b73ccb82740ed9':raise SystemExit('Unrecognized source')
s=raw.decode();s=s[s.index('      SUBROUTINE PYMULT(MMUL)'):]
def extract(start,end):return s[s.index(start):s.index(end,s.index(start))+len(end)]
init=extract('        YKE=SIGSUM/MAX(1D-10,SIGT(0,0,5))','        PIK=(VNT146/VNT147)*YKE')
sample=extract('          RTYPE=PYR(0)','          VINT(139)=SQRT(B2)/BAVG')
output.write_text("""      PROGRAM PROBE
      IMPLICIT DOUBLE PRECISION(A-H,O-Z)
      DIMENSION PARP(200),PARU(200),VINT(400),MSTP(200)
      DIMENSION SIGT(0:6,0:6,0:5)
      COMMON/TAPE/ U(2),ICURSOR
      PARU(1)=ACOS(-1D0)
      PARU(2)=2D0*PARU(1)
      MSTP(82)=4
   10 READ(*,*,END=20) PARP(83),PARP(84),SIGSUM,SIGT(0,0,5),U
      ICURSOR=0
"""+init+'\n'+sample+"""
      WRITE(*,'(10ES25.17)') SIGSUM,VNT146,VNT147,BAVG,BDIV,
     & PIK,B2,VINT(148),VINT(139),DBLE(ICURSOR)
      GOTO 10
   20 END
      DOUBLE PRECISION FUNCTION PYR(IDUMMY)
      IMPLICIT DOUBLE PRECISION(A-H,O-Z)
      COMMON/TAPE/ U(2),ICURSOR
      ICURSOR=ICURSOR+1
      IF(ICURSOR.GT.2) STOP 2
      PYR=U(ICURSOR)
      END
""")
