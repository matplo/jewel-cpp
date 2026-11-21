"""Extract unchanged PYWIDT gluon branch, with explicit decay-table inputs."""
from pathlib import Path
import hashlib,sys
source,output=map(Path,sys.argv[1:]);raw=source.read_bytes()
if hashlib.sha256(raw).hexdigest()!='9fcfed664e4ac34462aa63679f2dd3e935c9f6f63f7cd6a785b73ccb82740ed9':raise SystemExit('Unrecognized source')
s=raw.decode();s=s[s.index('      ELSEIF(KFLA.EQ.21) THEN',s.index('      SUBROUTINE PYWIDT')):];s=s[s.index('        DO 190 I=1,MDCY(KC,3)'):];end='  190   CONTINUE';s=s[:s.index(end)+len(end)]
output.write_text("""      PROGRAM PROBE
      IMPLICIT DOUBLE PRECISION(A-H,O-Z)
      DIMENSION PMAS(800,4),WIDS(400,5),MDME(400,2),KFDP(400,5)
      DIMENSION MDCY(1,3),WDTP(0:400),WDTE(0:400,0:5)
      KC=1
      MDCY(1,2)=1
   10 READ(*,*,END=20) SH,FUDGE,MDCY(1,3)
      DO 110 I=1,MDCY(1,3)
       READ(*,*) PMAS(I,1),PMAS(I+400,1),WIDS(I,1),MDME(I,1)
       KFDP(I,1)=I
       KFDP(I,2)=I+400
  110 CONTINUE
      WDTP=0D0
      WDTE=0D0
"""+s+"""
      SUMANN=WDTE(0,1)+WDTE(0,2)+WDTE(0,3)+WDTE(0,4)
      WRITE(*,*) WDTP(0),WDTE(0,:),SUMANN,(WDTP(I),I=1,MDCY(1,3))
      GOTO 10
   20 END
""")
