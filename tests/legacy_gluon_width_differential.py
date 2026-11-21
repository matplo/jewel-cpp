"""PYWIDT gluon partial widths, enable buckets and threshold decisions."""
import hashlib,json,math,subprocess,sys
from pathlib import Path
cpp,reference,output=sys.argv[1:]
cases=[]
masses=[.33,.33,.5,1.5,4.8,172.5,300,400]
energies={1.,10.,100.,1e4,1e6}
for mass in masses:
 edge=4*mass*mass
 energies.update([math.nextafter(edge,0),edge,math.nextafter(edge,math.inf),edge*(1-1e-10),edge*(1+1e-10)])
for sh in sorted(energies):
 for mode_pattern in [[1]*8,[-1]*8,[0]*8,[5]*8,[-1,0,1,2,3,4,5,1]]:
  for fudge in [0,.5,1,2]:
   for secondary in [0,.3,1]:
    decays=[[m,m,secondary,mode] for m,mode in zip(masses,mode_pattern)]
    cases.append((sh,fudge,decays))
# Unequal daughter masses exercise the two-mass opening condition separately
# from the source expression depending only on the first daughter.
for sh in [1,4,9,16,100]:
 cases.append((sh,1,[[.5,1.5,1,1],[1.5,.5,1,2]]))
for n in [0,1,9,400]:cases.append((1e6,1,[[0,0,.3,1]]*n))
def encode(rows):
 return ''.join(f'{s:.17g} {f:.17g} {len(d)}\n'+''.join(' '.join(format(x,'.17g') for x in row)+'\n' for row in d) for s,f,d in rows)
payload=encode(cases)
def run(exe):
 r=subprocess.run([exe],input=payload,capture_output=True,text=True,timeout=60)
 assert r.returncode==0,(exe,r.stderr)
 return [list(map(float,line.split())) for line in r.stdout.splitlines()]
a,b=run(cpp),run(reference);assert len(a)==len(b)==len(cases)
maximum=0.;values=0;failures=[]
for i,(left,right) in enumerate(zip(a,b)):
 assert len(left)==len(right)==8+len(cases[i][2])
 for col,(x,y) in enumerate(zip(left,right)):
  values+=1;maximum=max(maximum,abs(x-y))
  if not math.isfinite(x) or not math.isfinite(y) or not math.isclose(x,y,rel_tol=2e-12,abs_tol=2e-13) or ((x==0)!=(y==0)):
   failures.append(dict(case=i,column=col,cpp=x,fortran=y))
invalid=[(0,1,[]),(1,-1,[]),(1,1,[[-1,0,1,1]]),(1,1,[[0,0,-1,1]]),(1,1,[[0,0,1,6]]),(1,1,[[0,0,1,1]]*401)]
for case in invalid:
 r=subprocess.run([cpp],input=encode([case]),capture_output=True,text=True);assert r.returncode!=0,case
report=dict(scope='PYWIDT gluon branch with explicit decay data and fudge; general resonance logic excluded',cases=len(cases),scalar_comparisons=values,maximum_absolute_error=maximum,failures=failures,relative_tolerance=2e-12,absolute_tolerance=2e-13,zero_decisions_exact=True,invalid_cases=len(invalid),input_sha256=hashlib.sha256(payload.encode()).hexdigest(),source_sha256='9fcfed664e4ac34462aa63679f2dd3e935c9f6f63f7cd6a785b73ccb82740ed9',executables={Path(e).name:hashlib.sha256(Path(e).read_bytes()).hexdigest() for e in [cpp,reference]})
Path(output).write_text(json.dumps(report,indent=2)+'\n');assert not failures,failures[:10]
print('PASS',values,'gluon-width comparisons')
