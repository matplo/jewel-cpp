"""PUTPARTICLESONSHELL with injected common nominal masses and safe ranges."""
import hashlib,json,math,random,subprocess,sys
from pathlib import Path
cpp,reference,output=map(Path,sys.argv[1:]);rng=random.Random(260022);cases=[]
for mode in [0,1,2]:
 for first in [0,1]:
  for n in [2,3,8]:
   for scale in [.2,2,100]:
    for trial in range(4):
     rows=[]
     for i in range(n):
      px,py,pz=[rng.uniform(-scale,scale) for _ in range(3)];mass=rng.uniform(.01,.3)*scale
      rows.append([px,py,pz,math.sqrt(px*px+py*py+pz*pz+mass*mass),mass,rng.choice([0,.13957,.49367,.93827])])
     # A single selected particle has zero CMS spatial momentum and is outside
     # the stock Newton derivative's defined domain. Keep two selected entries.
     if mode==2 and n-first<2:continue
     cases.append((mode,first,rows))
cases.append((2,0,[[1,0,0,1.1,.1,5],[-1,0,0,1.1,.1,5]]))
# Threshold cases can exhaust the source's ten Newton steps; compare that
# bounded exit rather than silently treating it as convergence.
for gap in [1e-2,1e-8,1e-12]:
 mass=1.1*(1-gap)
 cases.append((2,0,[[1,0,0,1.1,.1,mass],[-1,0,0,1.1,.1,mass]]))
payload=''.join(f'{len(rows)} {mode} {first}\n'+''.join(' '.join(format(x,'.17g') for x in row)+'\n' for row in rows) for mode,first,rows in cases)
def run(exe):
 p=subprocess.run([str(exe.resolve())],input=payload,text=True,capture_output=True,check=True,timeout=30)
 lines=p.stdout.splitlines();offset=0;events=[]
 while offset<len(lines):
  n=int(lines[offset]);offset+=1;events.append([list(map(float,x.split())) for x in lines[offset:offset+n]]);offset+=n
 assert len(events)==len(cases)
 return events
actual,expected=run(cpp),run(reference);failures=[];values=0;maximum=0
for index,((mode,first,inputs),a,b) in enumerate(zip(cases,actual,expected)):
 assert len(a)==len(b)==len(inputs)
 for row,(x,y) in enumerate(zip(a,b)):
  if mode==0 or (mode==2 and row<first):assert x==inputs[row][:5]
  if mode==1:assert x[:3]==inputs[row][:3]
  if mode!=0 and not(mode==2 and row<first):
   assert x[4]==inputs[row][5]
   assert math.isclose(x[3]**2-sum(v*v for v in x[:3]),x[4]**2,abs_tol=1e-10*max(1,x[3]**2),rel_tol=1e-10)
  for col,(u,v) in enumerate(zip(x,y)):
   values+=1;maximum=max(maximum,abs(u-v))
   if not(math.isfinite(u) and math.isfinite(v) and math.isclose(u,v,rel_tol=2e-12,abs_tol=2e-13)):failures.append(dict(case=index,mode=mode,first=first,row=row,column=col,cpp=u,reference=v))
report=dict(scope='PUTPARTICLESONSHELL modes 0/1/2; injected nominal masses; safe ranges only',cases=len(cases),values=values,max_absolute_error=maximum,failures=failures,relative_tolerance=2e-12,absolute_tolerance=2e-13,input_sha256=hashlib.sha256(payload.encode()).hexdigest(),source=json.loads((Path(__file__).parents[1]/'docs/source-manifest.json').read_text())['jewel-2.6.0.f'],executables={str(p.resolve()):hashlib.sha256(p.read_bytes()).hexdigest() for p in [cpp,reference]})
output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items() if k!='failures'},indent=2))
if failures:print(failures[:5]);raise SystemExit(f'{len(failures)} projection mismatches')
