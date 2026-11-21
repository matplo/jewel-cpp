"""Unchanged PYMULT acceptance, pair flavour prescription and kinematics."""
from pathlib import Path
import hashlib,sys
source,output=map(Path,sys.argv[1:]);raw=source.read_bytes()
assert hashlib.sha256(raw).hexdigest()=='9fcfed664e4ac34462aa63679f2dd3e935c9f6f63f7cd6a785b73ccb82740ed9'
s=raw.decode();s=s[s.index('      SUBROUTINE PYMULT(MMUL)'):];s=s[:s.index('      SUBROUTINE ',30)]
start=s.index('        RFLAV=PYR(0)');end=s.index('        P(N+2,5)=0D0',start)+len('        P(N+2,5)=0D0')
body=s[start:end];accept='        IF(SIGS.LT.XSEC(ISUB,1)*PYR(0)) GOTO 220'
assert accept in s
output.write_text("""      PROGRAM PROBE
      IMPLICIT DOUBLE PRECISION(A-H,O-Z)
      DIMENSION P(2,5),K(2,5),VINT(400),PARP(200),PARJ(200),
     & PARU(200),XSEC(500,3)
      COMMON/TAPE/ U(4),ICURSOR
      N=0
      ISUB=96
   10 READ(*,*,END=30) SIGS,XSEC(ISUB,1),VINT(1),XT2,
     & VINT(41),VINT(42),VINT(23),PARP(85),PARP(86),PARJ(2),
     & PARU(2),U
      ICURSOR=0
"""+accept+'\n'+body+"""
      WRITE(*,'(15ES25.17)') 1D0,DBLE(ICURSOR),RFLAV,PT,PHI,
     & DBLE(K(1,2)),DBLE(K(2,2)),(P(1,J),J=1,4),(P(2,J),J=1,4)
      GOTO 10
  220 WRITE(*,'(15ES25.17)') 0D0,DBLE(ICURSOR),(0D0,J=1,13)
      GOTO 10
   30 END
      DOUBLE PRECISION FUNCTION PYR(IDUMMY)
      IMPLICIT DOUBLE PRECISION(A-H,O-Z)
      COMMON/TAPE/ U(4),ICURSOR
      ICURSOR=ICURSOR+1
      IF(ICURSOR.GT.4)STOP 2
      PYR=U(ICURSOR)
      END
""")
