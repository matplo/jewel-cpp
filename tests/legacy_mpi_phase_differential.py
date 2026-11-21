"""Massless MPI phase space and source momentum-availability boundaries."""
import hashlib,json,math,random,subprocess,sys
from pathlib import Path
cpp,reference,output=sys.argv[1:]
r=random.Random(2600421);cases=[]
for coefficients in [[0,0,0],[1,1,0],[.5,0,1],[.5,.25,.25],[.3,.2,.6]]:
 for xt in [1e-10,1e-6,.001,.1,.9,math.nextafter(1,0),1]:
  for _ in range(30):cases.append([xt,*coefficients,1,1,*[r.random() for _ in range(5)]])
  for col,edge in [(6,coefficients[0]),(8,coefficients[1]),(8,coefficients[1]+coefficients[2]),(10,.5)]:
   for value in [math.nextafter(edge,0),edge,math.nextafter(edge,1)]:
    if sys.float_info.min<=value<1:
     row=[xt,*coefficients,1,1,.4,.6,.4,.6,.4];row[col]=value;cases.append(row)
def encode(rows):return ''.join(' '.join(format(x,'.17g') for x in row)+'\n' for row in rows)
def run(exe,rows):
 result=subprocess.run([exe],input=encode(rows),capture_output=True,text=True,timeout=60)
 assert result.returncode==0,(exe,result.stderr)
 return [list(map(float,line.split())) for line in result.stdout.splitlines()]
# Construct availability edges using independently evaluated Fortran fractions.
seed_cases=cases[::max(1,len(cases)//150)];seed_outputs=run(reference,seed_cases)
for row,result in zip(seed_cases,seed_outputs):
 for beam,col in [(0,3),(1,4)]:
  edge=result[col]+.01
  if edge>=1:continue
  for value in [math.nextafter(edge,0),edge,math.nextafter(edge,1),edge-1e-10,edge+1e-10]:
   copy=list(row);copy[4+beam]=value;cases.append(copy)
a,b=run(cpp,cases),run(reference,cases);assert len(a)==len(b)==len(cases)
maximum=[0.]*7;failures=[]
for i,(left,right) in enumerate(zip(a,b)):
 assert len(left)==len(right)==7
 for col,(x,y) in enumerate(zip(left,right)):
  maximum[col]=max(maximum[col],abs(x-y))
  if not math.isfinite(x) or not math.isfinite(y) or not math.isclose(x,y,rel_tol=2e-12,abs_tol=2e-13) or (col>=5 and x!=y) or ((x==0)!=(y==0)):
   failures.append(dict(case=i,column=col,cpp=x,fortran=y,input=cases[i]))
 assert left[-1]==right[-1]==5
invalid=[]
base=[.01,.5,.25,.25,1,1,.4,.6,.4,.6,.4]
for col,value in [(0,0),(0,1.1),(1,-.1),(1,1.1),(2,.9),(4,-.1),(5,1.1),(6,0),(10,1)]:
 row=list(base);row[col]=value;invalid.append(row)
for row in invalid:
 result=subprocess.run([cpp],input=encode([row]),capture_output=True,text=True)
 assert result.returncode!=0,row
report=dict(scope='massless ISUB=96 hadron/hadron MPI phase-space proposal only',cases=len(cases),scalar_comparisons=7*len(cases),maximum_absolute_errors=maximum,failures=failures,relative_tolerance=2e-12,absolute_tolerance=2e-13,draws_per_proposal=5,availability_decisions_exact=not any(x['column']==5 for x in failures),available=sum(x[5]==1 for x in b),unavailable=sum(x[5]==0 for x in b),invalid_cases=len(invalid),input_sha256=hashlib.sha256(encode(cases).encode()).hexdigest(),source_sha256='9fcfed664e4ac34462aa63679f2dd3e935c9f6f63f7cd6a785b73ccb82740ed9',executables={Path(e).name:hashlib.sha256(Path(e).read_bytes()).hexdigest() for e in [cpp,reference]})
Path(output).write_text(json.dumps(report,indent=2)+'\n')
assert not failures,failures[:10]
print('PASS',report['scalar_comparisons'],'comparisons with exact availability and five draws')
