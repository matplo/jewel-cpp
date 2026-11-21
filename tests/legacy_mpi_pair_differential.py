"""PYMULT pair acceptance, draw order, flavour boundaries and four-momenta."""
import hashlib,json,math,random,subprocess,sys
from pathlib import Path
cpp,reference,output=sys.argv[1:]
rng=random.Random(2600421);cases=[]
def row(sigma=1,attach=.9,gluon=.9,strange=.3,draws=(.5,.5,.5,.5),x1=.1,x2=.2,c=.6,ecm=5020):
 return [sigma,1,ecm,x1*x2*(1-c*c),x1,x2,c,attach,gluon,strange,2*math.pi,*draws]
for _ in range(2000):
 cases.append(row(sigma=2*rng.random(),attach=rng.random(),gluon=rng.random(),strange=rng.random(),draws=[rng.random() for _ in range(4)],x1=.001+.998*rng.random(),x2=.001+.998*rng.random(),c=1.998*rng.random()-.999,ecm=rng.choice([200,2760,5020,13000])))
for attach,gluon in [(0,0),(.4,.9),(.9,.4),(1,1)]:
 threshold=max(attach,gluon)
 flavours=[.01,.5,.99]+[v for v in [math.nextafter(threshold,0),threshold,math.nextafter(threshold,1)] if sys.float_info.min<=v<1]
 for strange in [0,.3,1]:
  qdraws=[.01,.5,.99]
  for edge in [1/(2+strange),2/(2+strange)]:
   qdraws += [v for v in [math.nextafter(edge,0),edge,math.nextafter(edge,1)] if sys.float_info.min<=v<1]
  for flavour in flavours:
   for qdraw in qdraws:
    for sigma in [0,math.nextafter(.5,0),.5,math.nextafter(.5,1),2]:
     cases.append(row(sigma,attach,gluon,strange,(.5,flavour,.27,qdraw)))
def encode(rows):return ''.join(' '.join(format(v,'.17g') for v in r)+'\n' for r in rows)
payload=encode(cases)
def run(exe):
 p=subprocess.run([exe],input=payload,capture_output=True,text=True,timeout=30)
 assert p.returncode==0,(exe,p.stderr)
 values=[list(map(float,line.split())) for line in p.stdout.splitlines()]
 assert len(values)==len(cases) and all(len(r)==15 for r in values)
 return values
a,b=run(cpp),run(reference);failures=[];maximum=0.;counts={};closure=0.
for i,(x,y) in enumerate(zip(a,b)):
 for j,(u,v) in enumerate(zip(x,y)):
  maximum=max(maximum,abs(u-v))
  if not math.isfinite(u) or not math.isfinite(v) or not math.isclose(u,v,rel_tol=2e-12,abs_tol=2e-13) or (j in [0,1,5,6] and u!=v):
   failures.append(dict(case=i,column=j,cpp=u,fortran=v))
 counts[str(int(x[1]))]=counts.get(str(int(x[1])),0)+1
 if x[0]:
  ecm,x1,x2=cases[i][2],cases[i][4],cases[i][5]
  for got,expected in [(x[7]+x[11],0),(x[8]+x[12],0),(x[9]+x[13],.5*ecm*(x1-x2)),(x[10]+x[14],.5*ecm*(x1+x2))]:
   closure=max(closure,abs(got-expected)/ecm)
   assert abs(got-expected)<=2e-12*ecm
  for offset in [7,11]:
   px,py,pz,e=x[offset:offset+4]
   assert e>0 and abs(e*e-px*px-py*py-pz*pz)<=2e-12*ecm*ecm
invalid=[]
for column,value in [(0,-1),(1,0),(2,0),(3,0),(3,1.1),(4,0),(5,1.1),(6,1.1),(7,-.1),(8,1.1),(9,-.1),(9,1.1),(10,0)]:
 r=row();r[column]=value;invalid.append(r)
for r in invalid:
 p=subprocess.run([cpp],input=encode([r]),capture_output=True,text=True);assert p.returncode!=0,r
report=dict(scope='legacy PYMULT mode-6 proposal acceptance and pair flavour/kinematics; no colour attachment or showers',cases=len(cases),scalar_comparisons=15*len(cases),maximum_absolute_error=maximum,maximum_energy_scaled_closure_error=closure,draw_counts=counts,failures=failures,invalid_cases=len(invalid),relative_tolerance=2e-12,absolute_tolerance=2e-13,input_sha256=hashlib.sha256(payload.encode()).hexdigest(),source_sha256='9fcfed664e4ac34462aa63679f2dd3e935c9f6f63f7cd6a785b73ccb82740ed9',executables={Path(e).name:hashlib.sha256(Path(e).read_bytes()).hexdigest() for e in [cpp,reference]})
Path(output).write_text(json.dumps(report,indent=2)+'\n');assert not failures,failures[:10]
print('PASS',report['scalar_comparisons'],'comparisons; max absolute',maximum,'draw counts',counts)
