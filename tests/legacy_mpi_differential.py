"""PYMULT modes 4/5 weight parity, including retained tables at changed screening."""
import hashlib,json,math,random,subprocess,sys
from pathlib import Path
cpp,reference,output=sys.argv[1:]
record=json.loads((Path(__file__).parents[1]/'docs/validation-mpi-saved-state.json').read_text())['initializations'][0]
saved=record['sampling_bins'];norm=record['saved_normalization'][1]
# NMUL starts at MSTP(83); infer the captured initializer's value from its
# documented SIGSUM=sum(SIGM)/(20*MSTP(83)) identity, rather than fitting it.
n_init=round(sum(saved)/(20*record['saved_normalization'][0]))
assert n_init==100
cases=[]
def add(xt,screen,enhancement=2.,sigma=45.733890062687706,scale=1.,bypass=0,sums=saved,counts=None):
 cases.append(([xt,screen,enhancement,sigma,norm,scale,bypass],sums,counts or [n_init]*20))
for screen in [record['vint149'],record['vint149']*.81,1e-4,.1]:
 for coordinate in [i/20 for i in range(1,20)]:
  xt=screen*coordinate/(1+screen-coordinate)
  for value in [math.nextafter(xt,0),xt,math.nextafter(xt,1)]:add(value,screen)
 for xt in [0,1e-12,.01,1]:
  for enhancement in [0,1,100]:
   for sigma in [0,45.733890062687706]:
    for scale in [.5,1,2]:
     for bypass in [0,1]:add(xt,screen,enhancement,sigma,scale,bypass)
r=random.Random(2600421)
for i in range(1000):
 add(10**r.uniform(-12,0),10**r.uniform(-9,-1),r.uniform(0,10),r.uniform(0,100),r.uniform(.1,3),i%2,
     [r.uniform(0,50000) for _ in range(20)],[r.randint(1,1000) for _ in range(20)])
def encode(case):return '\n'.join(' '.join(map(str,row)) for row in case)+'\n'
payload=''.join(map(encode,cases))
def run(exe,data=payload):
 p=subprocess.run([exe],input=data,capture_output=True,text=True,check=True)
 return [list(map(float,line.split())) for line in p.stdout.splitlines()]
a,b=run(cpp),run(reference)
assert len(a)==len(b)==len(cases)
failures=[];maximum=[0.]*4
for i,(x,y) in enumerate(zip(a,b)):
 assert len(x)==len(y)==4
 for col,(u,v) in enumerate(zip(x,y)):
  maximum[col]=max(maximum[col],abs(u-v))
  if not math.isfinite(u) or not math.isfinite(v) or not math.isclose(u,v,rel_tol=2e-12,abs_tol=2e-13) or (col==1 and u!=v) or ((u==0)!=(v==0)):
   failures.append(dict(case=i,column=col,cpp=u,fortran=v))
# Deliberately changing only live screening must change the retained-table weight.
pair=[([1e-5,s,2.,45.733890062687706,norm,1.,0],saved,[100]*20) for s in [record['vint149'],record['vint149']*.81]]
pair_results=run(cpp,''.join(map(encode,pair)));assert pair_results[0][2:]!=pair_results[1][2:]
invalid=[]
base=pair[0]
for col,value in [(0,-1),(0,1.1),(1,0),(1,float('nan')),(2,-1),(3,-1),(4,0),(5,0)]:
 row=list(base[0]);row[col]=value;invalid.append((row,saved,[100]*20))
invalid+=[(base[0],[-1]+saved[1:],[100]*20),(base[0],saved,[0]+[100]*19)]
for case in invalid:
 result=subprocess.run([cpp],input=encode(case),capture_output=True,text=True)
 assert result.returncode!=0,case
report=dict(scope='legacy hard-overlap weights only, not connected to event generation',cases=len(cases),scalar_comparisons=4*len(cases),maximum_absolute_errors=maximum,relative_tolerance=2e-12,absolute_tolerance=2e-13,exact_bin_and_zero_decisions=True,failures=failures,invalid_cases=len(invalid),screening_only_pair=pair_results,inferred_initial_bin_count=n_init,input_sha256=hashlib.sha256(payload.encode()).hexdigest(),source_sha256='9fcfed664e4ac34462aa63679f2dd3e935c9f6f63f7cd6a785b73ccb82740ed9',executables={Path(e).name:hashlib.sha256(Path(e).read_bytes()).hexdigest() for e in [cpp,reference]})
Path(output).write_text(json.dumps(report,indent=2)+'\n')
assert not failures,failures[:10]
print('PASS',report['scalar_comparisons'],'comparisons and',len(invalid),'invalid inputs')
