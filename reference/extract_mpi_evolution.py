"""Compose unchanged PYMULT blocks, with supplied cross section and initial strings."""
from pathlib import Path
import hashlib,sys
source,output=map(Path,sys.argv[1:]);raw=source.read_bytes()
assert hashlib.sha256(raw).hexdigest()=='9fcfed664e4ac34462aa63679f2dd3e935c9f6f63f7cd6a785b73ccb82740ed9'
s=raw.decode();mult=s[s.index('      SUBROUTINE PYMULT(MMUL)'):];mult=mult[:mult.index('      SUBROUTINE ',30)]
def extract(text,start,end):
 a=text.index(start);b=text.index(end,a)+len(end);return text[a:b]
factor=extract(mult,'          XT2FAC=VNT146*VINT(148)*XSEC(ISUB,1)/','     &    MAX(1D-10,SIGT(0,0,5))*VINT(149)*(1D0+VINT(149))')
scale=extract(mult,'          IF(XT2.LE.0.01001D0*VINT(149)) GOTO 270','          XT2=MAX(0.01D0*VINT(149),XT2)')
phase=extract(mult,'        IF(PYR(0).LE.COEF(ISUB,1)) THEN','        IF(VINT(143)-X1M.LT.0.01D0.OR.VINT(144)-X2M.LT.0.01D0) GOTO 220')
phase=phase[phase.rindex('        IF(PYR(0).LE.COEF(ISUB,1)) THEN'):]
accept='        IF(SIGS.LT.XSEC(ISUB,1)*PYR(0)) GOTO 220';assert accept in mult
pair=extract(mult,'        RFLAV=PYR(0)','        P(N+2,5)=0D0')
a=mult.index('        IF(RFLAV.LT.PARP(85).AND.NSTR.GE.1) THEN');b=mult.index('C...Global statistics.',a);strings=mult[a:b]
stats=extract(mult,'        MINT(351)=MINT(351)+1','        IF (MINT(351).EQ.1) VINT(356)=PT')
update=extract(mult,'        MINT(31)=MINT(31)+1','        VINT(144)=VINT(144)-VINT(42)')
limits=extract(s[s.index('C...Special case for low-pT and multiple interactions:'):],'        VINT(12)=0.5D0*LOG(VINT(21))','        VINT(32)=-VINT(12)')
mapping=extract(s[s.index('      SUBROUTINE PYKMAP(IVAR,MVAR,VVAR)'):],'        YSTMIN=VINT(12)','        VINT(22)=MIN(YSTMAX,MAX(YSTMIN,YST))')
output.write_text("""      PROGRAM PROBE
      IMPLICIT DOUBLE PRECISION(A-H,O-Z)
      DIMENSION COEF(500,20),XSEC(500,3),SIGT(0:6,0:6,0:5),
     & P(1000,5),K(1000,5),KSTR(1000,2),MSTU(200),
     & PARP(200),PARJ(200),PARU(200),MINT(400)
      COMMON/VALUES/ VINT(400)
      COMMON/TAPE/ U(10),ICURSOR
      ISUB=96
      MSTU(5)=10000
   10 READ(*,*,END=90) VINT(1),XT2,VINT(149),VNT146,VINT(148),
     & XSEC(ISUB,1),SIGT(0,0,5),VINT(143),VINT(144),PARP(85),
     & PARP(86),PARJ(2),PARU(2),MINT(31),NSTEPS,
     & COEF(96,1),COEF(96,8),COEF(96,9)
      N=2
      NSTR=1
      P=0D0
      K=0
      KSTR(1,1)=1
      KSTR(1,2)=2
      K(1,2)=1
      K(2,2)=-1
      P(1,3)=10D0
      P(2,3)=-10D0
      P(1,4)=10D0
      P(2,4)=10D0
      VINT(151)=0D0
      VINT(152)=0D0
      VINT(351)=0D0
      VINT(356)=0D0
      MINT(351)=0
      ISTOP=0
"""+factor+"""
      DO ISTEP=1,NSTEPS
       READ(*,*) SIGS,U
       ICURSOR=0
       ICALLS=0
       IF(ISTOP.EQ.1)GOTO 270
"""+scale+"""
       ISTATUS=2
"""+phase+"""
C...Harness supplies PYSIGH output; its massless x fractions are these values.
       VINT(41)=X1M
       VINT(42)=X2M
       ICALLS=1
       ISTATUS=3
"""+accept+'\n'+pair+'\n'+strings+'\n'+stats+"""
        N=N+2
"""+update+"""
       ISTATUS=4
       IF(MINT(31).GE.240)ISTOP=1
       GOTO 220
  270  ISTATUS=1
       ISTOP=1
  220  WRITE(*,'(*(ES25.17))') DBLE(ISTATUS),DBLE(ICURSOR),
     & DBLE(ICALLS),XT2,VINT(143),VINT(144),VINT(151),VINT(152),
     & DBLE(MINT(31)),DBLE(MINT(351)),VINT(351),VINT(356),
     & DBLE(N),DBLE(NSTR),DBLE(ISTOP),
     & (DBLE(K(I,2)),(P(I,J),J=1,4),I=1,N),
     & ((DBLE(KSTR(I,J)),J=1,2),I=1,NSTR)
      ENDDO
      GOTO 10
   90 END
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
      COMMON/TAPE/ U(10),ICURSOR
      ICURSOR=ICURSOR+1
      IF(ICURSOR.GT.10)STOP 2
      PYR=U(ICURSOR)
      END
""")
