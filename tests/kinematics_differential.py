import hashlib,json,math,random,subprocess,sys
from pathlib import Path
cpp,reference,output=sys.argv[1:]
commands=[];labels=[]
r=random.Random(2604)
def row(op,values):commands.append(op+'\n'+' '.join(format(x,'.17g') for x in values)+'\n');labels.append(op)
for i in range(300):
 p=[r.uniform(-100,100) for _ in range(3)];p.append(math.sqrt(sum(x*x for x in p)+r.uniform(0,10)**2))
 v=[r.uniform(-5,5) for _ in range(4)];theta=r.uniform(-math.pi,math.pi);phi=r.uniform(-math.pi,math.pi)
 beta=[r.uniform(-.4,.4) for _ in range(3)]
 row('TRANSFORM',p+v+[theta,phi]+beta)
 row('OBSERVABLES',p+[r.uniform(-10,10)])
 row('POSITION',p+v+[r.uniform(-5,20)])
for p in [[0,0,0,0],[0,0,100,100],[0,0,-100,100],[-1,0,0,1],[-1,-0.,0,1],[1e-22,-1e-22,1e-22,1e-20],[100,100,100,1]]:
 row('OBSERVABLES',p+[0])
 for beta in [[0,0,0],[1e-11,0,0],[.999999,0,0],[1,0,0],[1.1,.2,0]]:
  for theta,phi in [(0,0),(1e-11,0),(.5,-.2)]:row('TRANSFORM',p+[1,2,3,4]+[theta,phi]+beta)
row('POSITION',[100,0,0,10,1,2,3,0,5])
payload=''.join(commands)
def run(exe):
 result=subprocess.run([exe],input=payload,text=True,capture_output=True,check=True)
 rows=[list(map(float,line.split())) for line in result.stdout.splitlines()]
 assert len(rows)==len(labels);return rows
actual,expected=run(cpp),run(reference);groups={};failures=[]
for i,(name,a,b) in enumerate(zip(labels,actual,expected)):
 assert len(a)==len(b)
 g=groups.setdefault(name,dict(values=0,max_absolute_error=0.,max_relative_error=0.))
 for col,(x,y) in enumerate(zip(a,b)):
  error=abs(x-y);g['values']+=1;g['max_absolute_error']=max(g['max_absolute_error'],error);g['max_relative_error']=max(g['max_relative_error'],error/max(abs(y),1e-300))
  if not math.isfinite(x) or not math.isclose(x,y,rel_tol=2e-12,abs_tol=2e-13):failures.append(dict(row=i,column=col,cpp=x,fortran=y))
report=dict(scope='active-particle Lorentz transforms, selected PYP fields and GETPOSITION',groups=groups,failures=failures,
 relative_tolerance=2e-12,absolute_tolerance=2e-13,input_sha256=hashlib.sha256(payload.encode()).hexdigest(),
 sources=json.loads((Path(__file__).parents[1]/'reference/kinematics-source.json').read_text()),
 executables={Path(exe).name:hashlib.sha256(Path(exe).read_bytes()).hexdigest() for exe in [cpp,reference]})
Path(output).write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(groups,indent=2))
if failures:print(failures[:10]);raise SystemExit('Kinematics parity failed')
print('PASS',sum(g['values'] for g in groups.values()),'comparisons')
