"""Screened PYMULT scale ordering, cutoff decisions and RNG consumption."""
import hashlib,json,math,random,subprocess,sys
from pathlib import Path
cpp,reference,output=sys.argv[1:]
cases=[]
for screening in [8.5884179958193529e-7,1.060298518002389e-6,.01]:
 edge=.01001*screening
 for upper in [0,.005*screening,math.nextafter(edge,0),edge,math.nextafter(edge,1),screening,.1,1]:
  for enhancement in [0,.01,2,100]:
   for bound in [0,500,1e5]:
    for u in [sys.float_info.min,.01,.5,.99,math.nextafter(1,0)]:
     cases.append([upper,screening,.83059932866738195,enhancement,bound,45.733890062687706,u])
# Explicitly cross the cancellation-to-zero and returned-floor boundaries.
for target in [0,.005,.01,.01001]:
 s=1e-6;upper=1e-4;a=.83;e=2;bound=500;nd=45
 factor=a*e*bound/nd*s*(1+s)
 u=math.exp((factor-factor*(upper+s)/(target*s+s))/(upper+s))
 for v in [math.nextafter(u,0),u,math.nextafter(u,1)]:cases.append([upper,s,a,e,bound,nd,v])
def encode(rows):return ''.join(' '.join(format(v,'.17g') for v in row)+'\n' for row in rows)
def run(exe,rows):
 result=subprocess.run([exe],input=encode(rows),capture_output=True,text=True,timeout=30)
 assert result.returncode==0,(exe,result.stderr)
 return [list(map(float,line.split())) for line in result.stdout.splitlines()]
# Build a sequence fixture from reference outputs, never native predicted scales.
r=random.Random(2600421);active={i:.1 for i in range(8)};sequence_counts=[0]*8
for step in range(500):
 if not active:break
 inputs=[[upper,1e-6,.83,2,500,45,r.random()] for upper in active.values()]
 values=run(reference,inputs);cases.extend(inputs)
 remaining={}
 for (key,upper),value in zip(active.items(),values):
  sequence_counts[key]+=1
  if value[2]:
   assert 0<value[1]<=upper
   remaining[key]=value[1]
 active=remaining
assert not active,'reference sequences did not terminate'
a,b=run(cpp,cases),run(reference,cases);assert len(a)==len(b)==len(cases)
maximum=[0.]*4;failures=[]
for i,(x,y) in enumerate(zip(a,b)):
 assert len(x)==len(y)==4
 for col,(u,v) in enumerate(zip(x,y)):
  maximum[col]=max(maximum[col],abs(u-v))
  if not math.isfinite(u) or not math.isfinite(v) or not math.isclose(u,v,rel_tol=2e-12,abs_tol=2e-13) or (col>=2 and u!=v) or ((u==0)!=(v==0)):
   failures.append(dict(case=i,column=col,cpp=u,fortran=v))
invalid=[]
base=[.1,1e-6,.83,2,500,45,.5]
for col,value in [(0,-1),(1,0),(2,0),(3,-1),(4,-1),(5,-1),(6,0),(6,1),(1,float('nan'))]:
 row=list(base);row[col]=value;invalid.append(row)
for row in invalid:
 result=subprocess.run([cpp],input=encode([row]),capture_output=True,text=True)
 assert result.returncode!=0,row
report=dict(scope='legacy MPI screened ordering proposal only; no differential kinematic/PDF acceptance',cases=len(cases),scalar_comparisons=4*len(cases),maximum_absolute_errors=maximum,failures=failures,relative_tolerance=2e-12,absolute_tolerance=2e-13,decision_and_random_draw_counts_exact=True,terminated_sequence_lengths=sequence_counts,pre_draw_stops=sum(x[3]==0 for x in a),post_draw_stops=sum(x[3]==1 and x[2]==0 for x in a),proposals=sum(x[2]==1 for x in a),invalid_cases=len(invalid),input_sha256=hashlib.sha256(encode(cases).encode()).hexdigest(),source_sha256='9fcfed664e4ac34462aa63679f2dd3e935c9f6f63f7cd6a785b73ccb82740ed9',executables={Path(e).name:hashlib.sha256(Path(e).read_bytes()).hexdigest() for e in [cpp,reference]})
Path(output).write_text(json.dumps(report,indent=2)+'\n')
assert not failures,failures[:10]
print('PASS',report['scalar_comparisons'],'comparisons, eight terminating scale sequences and',len(invalid),'invalid inputs')
