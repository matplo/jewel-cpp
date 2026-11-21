"""Extract unchanged PYZDIS shape statements; wrapper returns before sampling."""
import hashlib,sys
from pathlib import Path
source,output=map(Path,sys.argv[1:])
data=source.read_bytes()
if hashlib.sha256(data).hexdigest()!='9fcfed664e4ac34462aa63679f2dd3e935c9f6f63f7cd6a785b73ccb82740ed9':raise SystemExit('unknown Pythia6 source')
s=data.decode();start=s.index('      SUBROUTINE PYZDIS(');end=s.index('        MC=1',start)
block=s[start:end].replace('SUBROUTINE PYZDIS(KFL1,KFL2,PR,Z)','SUBROUTINE SHAPE_PROBE(KFL1,KFL2,PR,FA,FB,FC)')
output.write_text(block+'      ENDIF\n      RETURN\n      END\n')
