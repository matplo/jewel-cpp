import hashlib,json,math,random,subprocess,sys
from pathlib import Path
cpp,reference,output=sys.argv[1:]
commands=[];labels=[]
def config(nf=3):
 commands.append(f'CONFIG\n208 {nf} 1 .4 .59 .17 .17 .54 6.199999809265137 0 5 .45 1\n')
def tape(values): return str(len(values))+'\n'+' '.join(format(x,'.17g') for x in values)+'\n'
r=random.Random(2602)
config()
for lo,hi in [(0,10),(10,50),(50,90),(0,100)]:
 for u in [1e-10,.1,.5,.9,1-1e-10]:
  commands.append(f'EVENT\n{lo} {hi}\n'+tape([u]));labels.append('centrality')
for b in [0,3,7,12,18]:
 for i in range(10):
  values=[r.uniform(1e-12,1-1e-12) for _ in range(10000)]
  # First candidate attempts a corner; tests rejection and subsequent replay.
  values[:2]=[.999,.999]
  commands.append(f'VERTEX\n{b} 10\n'+tape(values));labels.append('vertex')
for nf in [0,3,5]:
 config(nf)
 for boost in [0,1]:
  for eta in [-2,0,2]:
   for species in [.01,.99]:
    for tau in [.4,1,3]:
     values=[species]+[r.uniform(1e-12,1-1e-12) for _ in range(10000)]
     commands.append(f'SCATTER\n0 0 {tau*math.sinh(eta):.17g} {tau*math.cosh(eta):.17g} {boost}\n'+tape(values));labels.append('scatterer')
 # The stock warning for cold points goes to stderr (logfid=0).
 commands.append('SCATTER\n30 0 0 1 1\n'+tape([.5]));labels.append('cold-fallback')
payload=''.join(commands)
def run(exe):
 result=subprocess.run([exe],input=payload,text=True,capture_output=True,check=True)
 rows=[list(map(float,line.split())) for line in result.stdout.splitlines()]
 if len(rows)!=len(labels):raise AssertionError((exe,len(rows),len(labels),result.stdout[:200]))
 return rows
actual,expected=run(cpp),run(reference);groups={};failures=[]
for i,(name,a,b) in enumerate(zip(labels,actual,expected)):
 assert len(a)==len(b)
 group=groups.setdefault(name,dict(cases=0,max_absolute_error=0.,max_relative_error=0.,max_draws=0))
 group['cases']+=1;group['max_draws']=max(group['max_draws'],int(a[-1]))
 if a[-1]!=b[-1]: failures.append(dict(row=i,reason='draw-count',cpp=a[-1],fortran=b[-1]))
 for col,(x,y) in enumerate(zip(a[:-1],b[:-1])):
  error=abs(x-y);group['max_absolute_error']=max(group['max_absolute_error'],error);group['max_relative_error']=max(group['max_relative_error'],error/max(abs(y),1e-300))
  if not math.isfinite(x) or not math.isclose(x,y,rel_tol=2e-12,abs_tol=2e-13):failures.append(dict(row=i,column=col,cpp=x,fortran=y))
 if name in ['scatterer','cold-fallback']:
  _,px,py,pz,energy,mass,_=a
  if abs(energy*energy-px*px-py*py-pz*pz-mass*mass)>2e-11*max(1,energy*energy):failures.append(dict(row=i,reason='mass shell'))
report=dict(scope='medium random-tape replay, not full events',groups=groups,failures=failures,
 input_sha256=hashlib.sha256(payload.encode()).hexdigest(),relative_tolerance=2e-12,absolute_tolerance=2e-13,
 sources=json.loads((Path(__file__).parents[1]/'reference/medium-source.json').read_text()),
 executables={Path(exe).name:hashlib.sha256(Path(exe).read_bytes()).hexdigest() for exe in [cpp,reference]})
Path(output).write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(groups,indent=2))
if failures:print(failures[:10]);raise SystemExit(f'{len(failures)} mismatches')
print('PASS',len(labels),'sampling cases with exact draw counts')
