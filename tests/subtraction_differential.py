"""Compare typed subtraction with unchanged JEWEL 2.6.0 C++ helpers, offset zero."""
import hashlib,json,math,random,subprocess,sys
from pathlib import Path
cpp,reference,output=map(Path,sys.argv[1:]);rng=random.Random(260021)
def momentum(pt,y,phi,mass):
 mt=math.sqrt(pt*pt+mass*mass)
 return [pt*math.cos(phi),pt*math.sin(phi),mt*math.sinh(y),mt*math.cosh(y)]
cases=[]
# Equal distances, exact radius boundary, complete consumption, residuals,
# phi wraparound and independent subtraction of transverse mass and momentum.
for radius in [0.,.1,.4,1.,4.,10.]:
 for n,m in [(1,1),(2,2),(6,3),(12,8)]:
  for case in range(5):
   parts=[[211 if i%2 else -211,*momentum(rng.uniform(.1,20),rng.uniform(-2,2),rng.uniform(-math.pi,math.pi),.14),.14] for i in range(n)]
   thermal=[momentum(rng.uniform(.1,5),rng.uniform(-2,2),rng.uniform(-math.pi,math.pi),.5) for _ in range(m)]
   cases.append((radius,parts,thermal))
for radius in [0,.4,math.nextafter(.4,0),math.nextafter(.4,math.inf)]:
 cases.append((radius,[[211,10,0,0,10,0],[-211,5,0,0,5,0]],[[3,0,0,3],[9,0,0,9]]))
 cases.append((radius,[[211,*momentum(10,0,0,.5),.5]], [momentum(2,0,.4,.5)]))
cases.append((.1,[[211,*momentum(10,.3,math.pi-.01,.14),.14]], [momentum(2,.3,-math.pi+.01,.5)]))
cases.append((.4,[],[]));cases.append((.4,[[211,2,0,0,2,0]],[]));cases.append((.4,[],[[1,0,0,1]]))
commands=[]
for i,(radius,parts,thermal) in enumerate(cases):
 commands.append(f'{len(parts)} {len(thermal)} {radius:.17g} {2600+i}\n')
 commands.extend(' '.join(format(x,'.17g') for x in row)+'\n' for row in parts+thermal)
payload=''.join(commands)
def run(exe):
 p=subprocess.run([str(exe.resolve())],input=payload,text=True,capture_output=True,check=True,timeout=30)
 rows=p.stdout.splitlines();events=[];i=0
 while i<len(rows):
  n=int(rows[i]);i+=1;events.append([list(map(float,row.split())) for row in rows[i:i+n]]);i+=n
 assert len(events)==len(cases)
 return events
actual,expected=run(cpp),run(reference);failures=[];values=0;maximum=0
for case,(a,b) in enumerate(zip(actual,expected)):
 if len(a)!=len(b):failures.append(dict(case=case,counts=[len(a),len(b)]));continue
 for row,(aa,bb) in enumerate(zip(a,b)):
  assert len(aa)==len(bb)==6
  for col,(x,y) in enumerate(zip(aa,bb)):
   values+=1;maximum=max(maximum,abs(x-y))
   if not(math.isfinite(x) and math.isfinite(y) and (x==y if col==0 else math.isclose(x,y,rel_tol=2e-12,abs_tol=2e-13))):failures.append(dict(case=case,row=row,column=col,cpp=x,reference=y))
report=dict(scope='subtraction.C helpers with explicit zero offset; not Fortran-facing wrapper or native event integration',cases=len(cases),values=values,max_absolute_error=maximum,failures=failures,relative_tolerance=2e-12,absolute_tolerance=2e-13,input_sha256=hashlib.sha256(payload.encode()).hexdigest(),source=json.loads((Path(__file__).parents[1]/'docs/source-manifest.json').read_text())['subtraction.C'],executables={str(p.resolve()):hashlib.sha256(p.read_bytes()).hexdigest() for p in [cpp,reference]})
output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items() if k not in ['executables','source']},indent=2))
if failures:raise SystemExit('Subtraction mismatch')
