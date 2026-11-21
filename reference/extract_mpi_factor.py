"""Extract the unchanged screened MPI common-factor expression from PYSIGH."""
from pathlib import Path
import hashlib,sys
source,output=map(Path,sys.argv[1:]);raw=source.read_bytes()
if hashlib.sha256(raw).hexdigest()!='9fcfed664e4ac34462aa63679f2dd3e935c9f6f63f7cd6a785b73ccb82740ed9':raise SystemExit('Unrecognized source')
s=raw.decode();start=s.index('C...Phase space integral for low-pT and multiple interactions');s=s[start:]
s=s[s.index('        COMFAC=PARU(1)*PARU(5)*FACK*0.5D0*VINT(2)/SH2'):]
end='     &  (1D0+VINT(149)))';s=s[:s.index(end)+len(end)]
output.write_text("""      PROGRAM PROBE
      IMPLICIT DOUBLE PRECISION(A-H,O-Z)
      DIMENSION PARU(200),VINT(400),COEF(500,20),MSTP(200)
      PARU(1)=ACOS(-1D0)
      MSTP(82)=4
      ISUBSV=96
   10 READ(*,*,END=20) VINT(2),XT2,TAU,YST,VINT(149),PARU(5),
     & FACK,COEF(96,1),COEF(96,2),COEF(96,8),COEF(96,9),COEF(96,10)
      SH=MAX(1D0,TAU*VINT(2))
      SH2=SH**2
      YSTMIN=0.5D0*LOG(TAU)
      YSTMAX=-YSTMIN
"""+s+"""
      WRITE(*,'(ES25.17)') COMFAC
      GOTO 10
   20 END
""")
