"""Replay live PYMULT overlap weights through the C++ port without altering events."""
import argparse,hashlib,json,math,subprocess
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('cpp',type=Path);p.add_argument('directory',type=Path);p.add_argument('baseline',type=Path);p.add_argument('output',type=Path);a=p.parse_args()
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
rows=[line.split() for line in (a.directory/'run.log').read_text().splitlines() if line.lstrip().startswith('MPI_WEIGHT_')]
assert len(rows)==4000
payload=[];expected=[]
for i in range(0,len(rows),4):
 group=rows[i:i+4]
 assert [x[0] for x in group]==['MPI_WEIGHT_INPUT','MPI_WEIGHT_SUMS','MPI_WEIGHT_COUNTS','MPI_WEIGHT_RESULT']
 assert [len(x)-1 for x in group]==[7,20,20,4]
 payload.extend(' '.join(x[1:])+'\n' for x in group[:3]);expected.append(list(map(float,group[3][1:])))
text=''.join(payload)
r=subprocess.run([str(a.cpp.resolve())],input=text,text=True,capture_output=True,check=True)
actual=[list(map(float,line.split())) for line in r.stdout.splitlines()];assert len(actual)==len(expected)
maximum=[0.]*4;failures=[]
for i,(left,right) in enumerate(zip(actual,expected)):
 assert len(left)==4
 for col,(x,y) in enumerate(zip(left,right)):
  maximum[col]=max(maximum[col],abs(x-y))
  if not math.isfinite(x) or not math.isfinite(y) or not math.isclose(x,y,rel_tol=2e-12,abs_tol=2e-13) or (col==1 and x!=y) or ((x==0)!=(y==0)):
   failures.append(dict(sample=i,column=col,cpp=x,fortran=y))
identity=sha(a.directory/'events.hepmc')==sha(a.baseline)
report=dict(scope='first 1000 PYMULT hard-overlap evaluations replayed through C++; not native event generation',evaluations=len(expected),scalar_comparisons=4*len(expected),maximum_absolute_errors=maximum,failures=failures,hepmc_byte_identical=identity,input_sha256=hashlib.sha256(text.encode()).hexdigest(),baseline_hepmc_sha256=sha(a.baseline),cpp_sha256=sha(a.cpp),artifacts={name:sha(a.directory/name) for name in ['pythia.f','reference','params.dat','medium.dat','run.log','instrumentation.patch']})
a.output.write_text(json.dumps(report,indent=2)+'\n')
assert identity and not failures,failures[:5]
print('PASS',len(expected),'live weights; full reference output byte-identical; maximum errors',maximum)
