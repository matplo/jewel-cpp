"""Frozen PYMULT double-Gaussian normalization and two-draw sampler replay."""
import hashlib,json,math,subprocess,sys
from pathlib import Path
cpp,reference,output=sys.argv[1:]
sigma=45.733890062687706
cases=[]
for fraction in [0,.2,.5,.8,1]:
 for radius in [.1,.4,1]:
  cuts=[(1-fraction)**2,(1-fraction)**2+2*fraction*(1-fraction)]
  types={.1,.5,.9}
  for edge in cuts:
   types.update(x for x in [math.nextafter(edge,0),edge,math.nextafter(edge,1)] if sys.float_info.min<=x<1)
  for ratio in [1.1,1.2,124.73191966455620/sigma,10]:
   for kind in sorted(types):
    for radial in [sys.float_info.min,1e-12,.5,math.nextafter(1,0)]:
     cases.append([fraction,radius,ratio*sigma,sigma,kind,radial])
# Exact saved reference parameters, independently checked against the live trace.
fixture=[.5,.4,124.73191966455620,sigma,.5,.5];cases.append(fixture)
def encode(rows):return ''.join(' '.join(format(x,'.17g') for x in row)+'\n' for row in rows)
payload=encode(cases)
def run(exe):
 result=subprocess.run([exe],input=payload,capture_output=True,text=True,timeout=120)
 assert result.returncode==0,(exe,result.stderr,len(result.stdout.splitlines()))
 return [list(map(float,line.split())) for line in result.stdout.splitlines()]
a,b=run(cpp),run(reference);assert len(a)==len(b)==len(cases)
maximum=[0.]*10;failures=[]
for i,(left,right) in enumerate(zip(a,b)):
 assert len(left)==len(right)==10
 for col,(x,y) in enumerate(zip(left,right)):
  maximum[col]=max(maximum[col],abs(x-y))
  if not math.isfinite(x) or not math.isfinite(y) or not math.isclose(x,y,rel_tol=2e-12,abs_tol=2e-13) or ((x==0)!=(y==0)):
   failures.append(dict(case=i,column=col,cpp=x,fortran=y))
 assert left[-1]==right[-1]==2
# Real initialization values independently captured by the full reference trace.
expected=[.83059932866738195,.091240840189946285]
assert all(math.isclose(x,y,rel_tol=2e-12,abs_tol=2e-13) for x,y in zip(a[-1][1:3],expected))
invalid=[]
for col,value in [(0,-.1),(0,1.1),(1,0),(1,1.1),(2,sigma),(2,float('inf')),(3,0),(4,0),(4,1),(5,0),(5,1)]:
 row=list(fixture);row[col]=value;invalid.append(row)
invalid.append([.5,.4,1.1e-12,1e-12,.5,.5])
for row in invalid:
 result=subprocess.run([cpp],input=encode([row]),capture_output=True,text=True,timeout=10)
 assert result.returncode!=0,row
report=dict(scope='legacy double-Gaussian normalization and hard-impact proposal, not full MPI generation',cases=len(cases),scalar_comparisons=len(cases)*10,maximum_absolute_errors=maximum,failures=failures,random_draws_per_sample=2,invalid_cases=len(invalid),live_initialization_fixture=a[-1],relative_tolerance=2e-12,absolute_tolerance=2e-13,input_sha256=hashlib.sha256(payload.encode()).hexdigest(),source_sha256='9fcfed664e4ac34462aa63679f2dd3e935c9f6f63f7cd6a785b73ccb82740ed9',executables={Path(e).name:hashlib.sha256(Path(e).read_bytes()).hexdigest() for e in [cpp,reference]})
Path(output).write_text(json.dumps(report,indent=2)+'\n')
assert not failures,failures[:10]
print('PASS',report['scalar_comparisons'],'comparisons; exactly two draws per proposal;',len(invalid),'invalid inputs')
