"""Copy unchanged PYMULT attachment block; compare ordered KSTR topology only."""
from pathlib import Path
import hashlib,sys
source,output=map(Path,sys.argv[1:]);raw=source.read_bytes()
assert hashlib.sha256(raw).hexdigest()=='9fcfed664e4ac34462aa63679f2dd3e935c9f6f63f7cd6a785b73ccb82740ed9'
s=raw.decode();s=s[s.index('      SUBROUTINE PYMULT(MMUL)'):]
a=s.index('        IF(RFLAV.LT.PARP(85).AND.NSTR.GE.1) THEN');b=s.index('C...Global statistics.',a)
output.write_text("""      PROGRAM PROBE
      IMPLICIT DOUBLE PRECISION(A-H,O-Z)
      DIMENSION P(1000,5),K(1000,5),KSTR(1000,2),PARP(200),MSTU(200)
      MSTU(5)=10000
   10 READ(*,*,END=30) N,NSTR,RFLAV,PARP(85),IPDG
      K=0
      P=0D0
      K(N+1,2)=IPDG
      DO I=1,N+2
       READ(*,*) (P(I,J),J=1,4)
      ENDDO
      DO I=1,NSTR
       READ(*,*) KSTR(I,1),KSTR(I,2)
      ENDDO
"""+s[a:b]+"""
      WRITE(*,'(*(I8))') NSTR,((KSTR(I,J),J=1,2),I=1,NSTR)
      GOTO 10
   30 END
""")
