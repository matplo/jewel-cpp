"""Extract unchanged ISUB=96 channel factors; PYWIDT sum is an external input."""
from pathlib import Path
import hashlib,sys
source,output=map(Path,sys.argv[1:]);raw=source.read_bytes()
if hashlib.sha256(raw).hexdigest()!='9fcfed664e4ac34462aa63679f2dd3e935c9f6f63f7cd6a785b73ccb82740ed9':raise SystemExit('Unrecognized source')
s=raw.decode();s=s[s.index('C...Multiple interactions: sum of QCD processes'):]
s=s[s.index('          FACQQ1=COMFAC*AS**2*4D0/9D0*(SH2+UH2)/TH2'):]
end='          SIGH(NCHN)=0.5D0*FACGG3';s=s[:s.index(end)+len(end)]
output.write_text("""      PROGRAM PROBE
      IMPLICIT DOUBLE PRECISION(A-H,O-Z)
      DIMENSION SIGH(1000),ISIG(1000,3),RAW(1000),WDTE(0:500,5)
      DIMENSION MSTP(200),PMAS(500,4),MDCY(500,3),MDME(8000,2)
      DIMENSION XSFX(2,-40:40),PA(-5:5),PB(-5:5)
      MDCY(21,2)=1
      WDTE(0,2)=0D0
      WDTE(0,3)=0D0
      WDTE(0,4)=0D0
   10 READ(*,*,END=20) SH,TH,UH,CTH,COMFAC,AS,FACA,WDTE(0,1),
     & MSTP(34),(PMAS(I,1),I=1,5),(MDME(I,1),I=1,5),PA,PB
      SH2=SH**2
      TH2=TH**2
      UH2=UH**2
      NCHN=0
"""+s+"""
      DO 400 I=-5,5
       J=I
       IF(I.EQ.0) J=21
       XSFX(1,J)=PA(I)
       XSFX(2,J)=PB(I)
  400 CONTINUE
      SIGS=0D0
      DO 410 ICHN=1,NCHN
       RAW(ICHN)=SIGH(ICHN)
       KFL1=ISIG(ICHN,1)
       SIGH(ICHN)=SIGH(ICHN)*XSFX(1,KFL1)
       KFL2=ISIG(ICHN,2)
       SIGH(ICHN)=SIGH(ICHN)*XSFX(2,KFL2)
       SIGS=SIGS+SIGH(ICHN)
  410 CONTINUE
      WRITE(*,*) NCHN,SIGS
      DO 420 I=1,NCHN
       WRITE(*,*) ISIG(I,:),RAW(I),SIGH(I)
  420 CONTINUE
      GOTO 10
   20 END
""")
