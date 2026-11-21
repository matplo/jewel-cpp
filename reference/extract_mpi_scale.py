"""Extract unchanged PYMULT mode-6 screened ordering statements."""
from pathlib import Path
import hashlib,sys
source,output=map(Path,sys.argv[1:]);raw=source.read_bytes()
if hashlib.sha256(raw).hexdigest()!='9fcfed664e4ac34462aa63679f2dd3e935c9f6f63f7cd6a785b73ccb82740ed9':raise SystemExit('Unrecognized source')
s=raw.decode();s=s[s.index('      SUBROUTINE PYMULT(MMUL)'):]
def extract(start,end):return s[s.index(start):s.index(end,s.index(start))+len(end)]
factor=extract('          XT2FAC=VNT146*VINT(148)*XSEC(ISUB,1)/','     &    MAX(1D-10,SIGT(0,0,5))*VINT(149)*(1D0+VINT(149))')
proposal=extract('          IF(XT2.LE.0.01001D0*VINT(149)) GOTO 270','          XT2=MAX(0.01D0*VINT(149),XT2)')
output.write_text("""      PROGRAM PROBE
      IMPLICIT DOUBLE PRECISION(A-H,O-Z)
      DIMENSION VINT(400),XSEC(500,3),SIGT(0:6,0:6,0:5)
      COMMON/TAPE/ U,ICURSOR
      ISUB=96
   10 READ(*,*,END=20) XT2,VINT(149),VNT146,VINT(148),
     & XSEC(ISUB,1),SIGT(0,0,5),U
      ICURSOR=0
"""+factor+'\n'+proposal+"""
      IPROP=1
      GOTO 280
  270 IPROP=0
  280 WRITE(*,'(4ES25.17)') XT2FAC,XT2,DBLE(IPROP),DBLE(ICURSOR)
      GOTO 10
   20 END
      DOUBLE PRECISION FUNCTION PYR(IDUMMY)
      IMPLICIT DOUBLE PRECISION(A-H,O-Z)
      COMMON/TAPE/ U,ICURSOR
      ICURSOR=ICURSOR+1
      IF(ICURSOR.GT.1) STOP 2
      PYR=U
      END
""")
