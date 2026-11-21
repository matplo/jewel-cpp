"""Build isolated stock JEWEL vacuum baseline and ISR timing trace binaries.

The only baseline change removes a duplicate WEIGHT declaration already
normalized by the reference harness. Trace calls write existing record fields
without drawing random numbers or evaluating additional Pythia functions.
"""
from pathlib import Path
import argparse,shutil,hashlib,json,subprocess,sys
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('source',type=Path,help='Frozen JEWEL 2.6.0 source directory')
parser.add_argument('output',type=Path,help='New isolated build directory')
parser.add_argument('--lhapdf-prefix',required=True,type=Path)
args=parser.parse_args()
source=args.source.resolve();out=args.output.resolve()
out.mkdir(exist_ok=False)
for name in ['pythia6425mod-lhapdf6.f','meix.f','subtraction.C']:
 shutil.copyfile(source/name,out/name)
shutil.copyfile(source/'medium-vac.f.stock',out/'medium-vac.f')
s=(source/'jewel-2.6.0.f.stock').read_text()
assert hashlib.sha256((source/'jewel-2.6.0.f.stock').read_bytes()).hexdigest()=='78b39f19513c3633c00d45b22f0803c0873a93d98a536d66efc9277255bf5b81'
normalization=('TAUEST,QSUMVECOLD(4),ZDUM,WEIGHT,','TAUEST,QSUMVECOLD(4),ZDUM,')
assert s.count(normalization[0])==1
s=s.replace(*normalization)
(out/'baseline.f').write_text(s)
needle='C--walk backwards from ME to beam and set production vertices and times'
s=s.replace(needle,needle+'\n      CALL JISR_BEGIN')
needle='\t      lprev=lstart'
assert s.count(needle)==1
s=s.replace(needle,'      CALL JISR_LINE(1,I,LSTART)\n'+needle)
needle='\t\t  lprev=l1'
assert s.count(needle)==1
s=s.replace(needle,'      CALL JISR_LINE(2,I,L1)\n      CALL JISR_LINE(3,I,L2)\n'+needle)
assert s.count('CALL JISR_LINE')==3
s+='''
      SUBROUTINE JISR_BEGIN
      IMPLICIT NONE
      INTEGER ATTEMPT
      COMMON/JISR_COUNT/ATTEMPT
      LOGICAL FIRST
      SAVE FIRST
      DATA FIRST/.TRUE./
      IF(FIRST)THEN
        ATTEMPT=0
        OPEN(81,FILE='isr-lines.csv',STATUS='NEW')
        WRITE(81,'(A)')
     & 'attempt,role,side,line,parent,pdg,px,py,pz,energy,mass,x,y,z,production_time,end_time'
        FIRST=.FALSE.
      ENDIF
      ATTEMPT=ATTEMPT+1
      END
      SUBROUTINE JISR_LINE(ROLE,SIDE,LINE)
      IMPLICIT NONE
      INTEGER ROLE,SIDE,LINE,ATTEMPT,N,NPAD,K,J
      DOUBLE PRECISION P,V,MV
      COMMON/TIME/MV(23000,5)
      COMMON/JISR_COUNT/ATTEMPT
      COMMON/PYJETS/N,NPAD,K(23000,5),P(23000,5),V(23000,5)
      WRITE(81,100)ATTEMPT,ROLE,SIDE,LINE,K(LINE,3),K(LINE,2),
     & (P(LINE,J),J=1,5),(MV(LINE,J),J=1,5)
 100  FORMAT(I0,5(',',I0),10(',',ES26.17E3))
      END
'''
(out/'jewel-2.6.0.f').write_text(s)
commands=[['gfortran','-O2','-std=legacy','-ffixed-line-length-none','-c',name] for name in ['jewel-2.6.0.f','medium-vac.f','pythia6425mod-lhapdf6.f','meix.f']]
commands+=[['c++','-c','subtraction.C'],['gfortran','-o','jewel-vac-trace','jewel-2.6.0.o','medium-vac.o','pythia6425mod-lhapdf6.o','meix.o','subtraction.o','-L'+str(args.lhapdf_prefix.resolve()/'lib'),'-lLHAPDF','-lc++' if sys.platform=='darwin' else '-lstdc++']]
for command in commands:subprocess.run(command,cwd=out,check=True)
commands.append(['gfortran','-O2','-std=legacy','-ffixed-line-length-none','-c','baseline.f'])
baseline_link=commands[-2].copy();baseline_link[2]='jewel-vac-baseline';baseline_link[3]='baseline.o'
commands.append(baseline_link)
for command in commands[-2:]:subprocess.run(command,cwd=out,check=True)
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
manifest=dict(scope='Diagnostic of stock interpretpythiaevent ISR energies and times',
 commands=commands,normalization=dict(old=normalization[0],new=normalization[1]),
 sources={name:sha(source/name) for name in ['jewel-2.6.0.f.stock','medium-vac.f.stock','pythia6425mod-lhapdf6.f','meix.f','subtraction.C']},
 artifacts={name:sha(out/name) for name in ['jewel-2.6.0.f','baseline.f','jewel-vac-trace','jewel-vac-baseline']},
 builder_sha256=sha(Path(__file__)))
(out/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
print(out/'jewel-vac-trace')
