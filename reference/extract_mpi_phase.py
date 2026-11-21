"""Extract PYMULT massless tau/angle and hadronic rapidity mapping statements."""
from pathlib import Path
import hashlib,sys
source,output=map(Path,sys.argv[1:]);raw=source.read_bytes()
if hashlib.sha256(raw).hexdigest()!='9fcfed664e4ac34462aa63679f2dd3e935c9f6f63f7cd6a785b73ccb82740ed9':raise SystemExit('Unrecognized source')
s=raw.decode()
def extract(text,start,end):return text[text.index(start):text.index(end,text.index(start))+len(end)]
mult=s[s.index('      SUBROUTINE PYMULT(MMUL)'):]
# Production's indentation uniquely identifies this block rather than initialization.
phase=extract(mult,'        IF(PYR(0).LE.COEF(ISUB,1)) THEN','        IF(VINT(143)-X1M.LT.0.01D0.OR.VINT(144)-X2M.LT.0.01D0) GOTO 220')
# Select the production block explicitly (the first similarly indented earlier
# branch lacks the momentum-availability check).
phase=phase[phase.rindex('        IF(PYR(0).LE.COEF(ISUB,1)) THEN'):]
limits=extract(s[s.index('C...Special case for low-pT and multiple interactions:'):],
 '        VINT(12)=0.5D0*LOG(VINT(21))','        VINT(32)=-VINT(12)')
mapping=extract(s[s.index('      SUBROUTINE PYKMAP(IVAR,MVAR,VVAR)'):],
 '        YSTMIN=VINT(12)','        VINT(22)=MIN(YSTMAX,MAX(YSTMIN,YST))')
output.write_text("""      PROGRAM PROBE
      IMPLICIT DOUBLE PRECISION(A-H,O-Z)
      DIMENSION COEF(500,20)
      COMMON/VALUES/ VINT(400)
      COMMON/TAPE/ U(5),ICURSOR
      ISUB=96
   10 READ(*,*,END=20) XT2,COEF(96,1),COEF(96,8),COEF(96,9),
     & VINT(143),VINT(144),U
      ICURSOR=0
"""+phase+"""
      IAVAILABLE=1
      GOTO 230
  220 IAVAILABLE=0
  230 WRITE(*,'(7ES25.17)') TAU,VINT(22),VINT(23),X1M,X2M,
     & DBLE(IAVAILABLE),DBLE(ICURSOR)
      GOTO 10
   20 END
      SUBROUTINE PYKLIM(ILIM)
      IMPLICIT DOUBLE PRECISION(A-H,O-Z)
      COMMON/VALUES/ VINT(400)
"""+limits+"""
      END
      SUBROUTINE PYKMAP(IVAR,MVAR,VVAR)
      IMPLICIT DOUBLE PRECISION(A-H,O-Z)
      COMMON/VALUES/ VINT(400)
      DIMENSION MINT(400)
      MINT(47)=4
      ISTSB=9
"""+mapping+"""
      END
      DOUBLE PRECISION FUNCTION PYR(IDUMMY)
      IMPLICIT DOUBLE PRECISION(A-H,O-Z)
      COMMON/TAPE/ U(5),ICURSOR
      ICURSOR=ICURSOR+1
      IF(ICURSOR.GT.5) STOP 2
      PYR=U(ICURSOR)
      END
""")
