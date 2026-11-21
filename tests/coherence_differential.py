"""Replay DOCOHSCATTERING arithmetic sections; not a full-routine parity test."""
import hashlib,json,math,random,struct,subprocess,sys
from pathlib import Path
cpp,reference,output=sys.argv[1:]
rng=random.Random(260026)
cases=[]
def add(op,values):cases.append((op,values))
for p in [(0,0,20,21),(2,3,4,6),(-9,1,-3,11),(0,0,0,1)]:
 for q in [(-.1,.2,.3,.4),(-4,2,-1,5),(0,0,0,0),(-100,0,0,3)]:
  for factor in [0,1,2]:
   for angle in [0,.05,.7]:
    for t in [.4,1,20]:add('GATE',list(p)+list(q)+[factor,angle,t,.4])
# Resolve on both sides and at the literal threshold, including strict equality.
f32=lambda x:struct.unpack('f',struct.pack('f',x))[0]
for separation in [.01,1,10]:
 threshold=f32(.2)/separation
 for q in [math.nextafter(threshold,0),threshold,math.nextafter(threshold,math.inf)]:
  add('GATE',[0,0,20,21,-q,0,0,7,1,1,separation,0])
for _ in range(300):
 p=[rng.uniform(-30,30) for _ in range(3)];p.append(math.sqrt(sum(x*x for x in p)+4))
 q=[-rng.uniform(0,10)]+[rng.uniform(-4,4) for _ in range(3)]
 add('GATE',p+q+[rng.uniform(.1,3),rng.uniform(.001,1),rng.uniform(.5,15),.4])
for mass in [0,1,4,12]:
 for ma,mb in [(0,0),(.3,.5),(1,2),(6,8)]:
  for magnitude in [1,10,100]:
   for theta,phi in [(0,0),(.8,1.1),(2.6,-2)]:
    p=[magnitude*math.sin(theta)*math.cos(phi),magnitude*math.sin(theta)*math.sin(phi),magnitude*math.cos(theta),math.sqrt(magnitude*magnitude+mass*mass)]
    for z in [.01,.2,.5,.9,.99]:
     add('SPLIT',p+[mass,1,-2,3,.4,ma,mb,z,.7])
add('SPLIT',[0,0,0,4,4,1,2,3,.4,0,0,.5,.7])
payload=''.join(op+'\n'+' '.join(format(x,'.17g') for x in v)+'\n' for op,v in cases)
def run(exe):
 r=subprocess.run([exe],input=payload,text=True,capture_output=True,check=True)
 rows=[list(map(float,line.split())) for line in r.stdout.splitlines()]
 assert len(rows)==len(cases),(exe,len(rows),len(cases))
 return rows
a,b=run(cpp),run(reference);groups={};failures=[];undefined=[];conservation=0.
for i,((op,inputs),actual,expected) in enumerate(zip(cases,a,b)):
 g=groups.setdefault(op,dict(cases=0,finite_cases=0,values=0,max_absolute_error=0.,accepted=0,rejected=0))
 g['cases']+=1
 if actual[0]!=expected[0]:failures.append(dict(case=i,reason='domain classification',actual=actual,expected=expected));continue
 if expected[0]:undefined.append(i);continue
 g['finite_cases']+=1
 flagcol=4 if op=='GATE' else 1
 if actual[flagcol]!=expected[flagcol]:failures.append(dict(case=i,reason='branch mismatch'))
 accepted=(actual[flagcol]==0) if op=='GATE' else bool(actual[flagcol])
 g['accepted' if accepted else 'rejected']+=1
 assert len(actual)==len(expected)
 for j,(x,y) in enumerate(zip(actual[1:],expected[1:]),1):
  if math.isinf(y):
   if x!=y:failures.append(dict(case=i,column=j,reason='infinite threshold mismatch'))
   continue
  error=abs(x-y);g['values']+=1;g['max_absolute_error']=max(g['max_absolute_error'],error)
  if not math.isfinite(x) or not math.isclose(x,y,rel_tol=2e-12,abs_tol=2e-13):failures.append(dict(case=i,column=j,cpp=x,fortran=y))
 if op=='SPLIT' and accepted:
  # Independently check conservation against the input, not the rounded parent.
  residual=max(abs(actual[8+j]+actual[12+j]-inputs[j]) for j in range(4))
  conservation=max(conservation,residual)
  if residual>2e-12*max(1,inputs[3]):failures.append(dict(case=i,reason='momentum conservation',residual=residual))
report=dict(scope='DOCOHSCATTERING resolution and daughter-momentum sections only',groups=groups,
 undefined_reference_cases=undefined,max_conservation_residual=conservation,failures=failures,
 relative_tolerance=2e-12,absolute_tolerance=2e-13,input_sha256=hashlib.sha256(payload.encode()).hexdigest(),
 sources=json.loads((Path(__file__).parents[1]/'reference/coherence-source.json').read_text()),
 executables={Path(exe).name:hashlib.sha256(Path(exe).read_bytes()).hexdigest() for exe in [cpp,reference]})
Path(output).write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(groups,indent=2));print('undefined',len(undefined),'conservation',conservation)
if failures:print(failures[:20]);raise SystemExit(f'{len(failures)} mismatches')
