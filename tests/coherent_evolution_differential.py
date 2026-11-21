"""Complete DOCOHSCATTERING replay against unchanged JEWEL 2.6.0."""
import hashlib,json,math,random,subprocess,sys
from pathlib import Path
cpp,reference,output=sys.argv[1:]
def row(v):return ' '.join(format(x,'.17g') for x in v)+'\n'
base='CONFIG\n208 3 1 .4 .59 .17 .17 .54 6.199999809265137 0 2 .45 1\nFCONFIG\n3 1 .2 100 1 0\nRCONFIG\n3 1 .2 100 1 0\nXCONFIG\n3 1 .2 .2 100 1 0\nQCONFIG\n3 .2 .2 0\nBCONFIG\n3 1 .2 1 0\n'
cases=[];labels=[];capacity=64;active_nf=3;active_boost=0

def rec(status,pdg,parent,daughters,p,mass,colour,anti):
 return [status,pdg,parent,*daughters,*p,mass,0,0,0,0,0,0,0,.4,8,.4,.3,0,0,colour,anti,.5]
def append(pdg,first,second,direction,selected,left,cross,length,kinmode=1,history=0,recmode=0,subtract=0,energy=20,mass=4,rotation=0,shorten=False,weight=1,enabled=1,near_edge=False):
 momentum=math.sqrt(energy*energy-mass*mass);z=.4;ma=.3;mb=.5
 if near_edge:
  rest_energy=(mass*mass+mb*mb-ma*ma)/(2*mass)
  rest_momentum=math.sqrt(rest_energy*rest_energy-mb*mb)
  z=(rest_energy-momentum/energy*rest_momentum)/mass+1e-7
 pz=(2*z*energy*energy-mass*mass-mb*mb+ma*ma)/(2*momentum)
 pt=math.sqrt(z*z*energy*energy-pz*pz-mb*mb)
 def rotate(p):
  x,y,z,e=p;theta=rotation;phi=.7 if rotation else 0
  x,z=math.cos(theta)*x+math.sin(theta)*z,-math.sin(theta)*x+math.cos(theta)*z
  return [math.cos(phi)*x-math.sin(phi)*y,math.sin(phi)*x+math.cos(phi)*y,z,e]
 parent=rotate([0,0,momentum,energy])
 records=[rec(0,0,0,[0,0],[0,0,0,0],0,0,0),rec(11,21,0,[0,0],parent,mass,101,102),
  rec(11,pdg,2,[4,5],parent,mass,101,102),rec(1,first,3,[0,0],rotate([-pt,0,momentum-pz,(1-z)*energy]),ma,101 if direction>=0 else 103,104),
  rec(1,second,3,[0,0],rotate([pt,0,pz,z*energy]),mb,104,102)]
 records[0]=[0]*26;records[0][19]=-1
 if shorten:records[4 if selected==4 else 3][18]=1.2
 seed=260000+len(cases);rng=random.Random(seed);tape=[rng.uniform(.000001,.999999) for _ in range(20000)]
 config=f'ECONFIG\n1 1 1 20 {cross}\nCOHERENT\n'+row([selected,enabled,1,left,.05,.4,length,kinmode,110,weight,history])
 config+=''.join(row(r) for r in records)+row([1,0,0,1,1,recmode,1,subtract,capacity])
 for _ in range(history):config+=row([-.2,.1,.2,.3,.01,.6,21,0,0,0,1,1,0,0,0,.6])
 config+='20000\n'+row(tape)
 cases.append(config);labels.append(dict(pdg=pdg,selected=selected,left=left,cross=cross,length=length,seed=seed,kinmode=kinmode,history=history,recmode=recmode,subtract=subtract,energy=energy,mass=mass,rotation=rotation,shorten=shorten,weight=weight,enabled=enabled,near_edge=near_edge,nf=active_nf,boost=active_boost))
for pdg,first,second,direction in [(21,21,21,1),(21,21,21,-1),(21,1,-1,0),(1,21,1,0),(-1,21,-1,0)]:
 for selected in [4,5]:
  for left,cross,length in [(0,1,1),(.5,0,1),(.5,1,1),(1,10,1),(1,10,0)]:
   append(pdg,first,second,direction,selected,left,cross,length)
for kinmode in range(4):
 for history in [0,1,2]:
  for recmode in range(4):
   append(21,21,21,1,4,1,10,.01,kinmode,history,recmode,1,rotation=.8,shorten=True,weight=-.5)
for i in range(12):append(1,21,1,0,5,2,10,0,energy=6,mass=4)
for i in range(8):append(1,21,1,0,5,1,10,0,near_edge=True)
append(21,21,21,1,4,1,1,1,enabled=0)
# A second configuration exercises boosted thermal centres and NF=5.
second_start=len(cases);active_nf=5;active_boost=1
for i in range(12):
 append(21,21,21,1,4 if i%2 else 5,1,5,.01,i%4,i%3,i%4,i%2,rotation=.8 if i%2 else 2.6)
second_base='CONFIG\n208 5 1 .4 .59 .17 .17 .54 6.199999809265137 0 2 .45 1\nFCONFIG\n5 1 .2 100 1 1\nRCONFIG\n5 1 .2 100 1 1\nXCONFIG\n5 1 .2 .2 100 1 1\nQCONFIG\n5 .2 .2 1\nBCONFIG\n5 1 .2 1 5\n'
cases[second_start]=second_base+cases[second_start]
payload=base+''.join(cases)
def run(exe):
 r=subprocess.run([exe],input=payload,text=True,capture_output=True,timeout=120)
 if r.returncode:raise RuntimeError((exe,r.returncode,r.stderr[-3000:]))
 rows=iter(r.stdout.splitlines());out=[]
 for _ in cases:
  h=list(map(float,next(rows).split()));n=int(h[0]);out.append([h]+[list(map(float,next(rows).split())) for _ in range(n+103+capacity)])
 assert next(rows,None) is None
 stats=[];errors=[];error=None
 for line in r.stderr.splitlines():
  if line.startswith('COH_ERROR '):error=line.removeprefix('COH_ERROR ')
  if line.startswith('COH_STATS '):stats.append(list(map(int,line.split()[1:])));errors.append(error);error=None
 return out,stats,errors
(a,stats,errors),(b,_,_)=run(cpp),run(reference);failures=[];invalid=[];finite=0;values=0;maximum=0.;maxdraws=0;maxrecords=0;withkicks=0
for index,(actual,expected) in enumerate(zip(a,b)):
 if expected[0][6] or actual[0][6]:
  invalid.append(dict(case=index,cpp=int(actual[0][6]),fortran=int(expected[0][6]),reason=errors[index],nonfinite_reference_fields=[(i,j) for i,r in enumerate(expected) for j,x in enumerate(r) if not math.isfinite(x)]))
  n=int(expected[0][0])
  if expected[0][6]!=1 or actual[0][6]!=1 or errors[index]!='nonphysical recoil dummy projection':failures.append(dict(case=index,reason='unmatched domain failure'))
  if not all(n+102<=i<=n+101+capacity and j==10 for i,j in invalid[-1]['nonfinite_reference_fields']):failures.append(dict(case=index,reason='unreviewed nonfinite field'))
  continue
 finite+=1;maxdraws=max(maxdraws,int(expected[0][4]));maxrecords=max(maxrecords,int(expected[0][0]));withkicks+=expected[0][2]>0
 if actual[0]!=expected[0]:failures.append(dict(case=index,reason='header',cpp=actual[0],fortran=expected[0]));continue
 n=int(expected[0][0])
 for i,(ar,br) in enumerate(zip(actual,expected)):
  assert len(ar)==len(br),(index,i,len(ar),len(br))
  for j,(x,y) in enumerate(zip(ar,br)):
   values+=1;error=abs(x-y);maximum=max(maximum,error)
   discrete=i==0 or (1<=i<=n and (j<5 or 21<=j<=24)) or (n+2<=i<=n+101 and j==6) or (n+102<=i<=n+101+capacity and j==0)
   # Saved transfer four-vectors subtract comparable boosted momenta, after
   # feedback through many kicks. Keep this budget separate from all event fields.
   transfer_momentum=(i==n+1 and j<4) or (n+2<=i<=n+101 and 1<=j<=4)
   equal=x==y if discrete else math.isclose(x,y,rel_tol=2e-12,abs_tol=2e-11 if transfer_momentum else 2e-13)
   if not math.isfinite(x) or not equal:failures.append(dict(case=index,row=i,column=j,cpp=x,fortran=y))
report=dict(scope='full DOCOHSCATTERING replay',cases=len(cases),finite_cases=finite,values=values,max_absolute_error=maximum,
 max_draws=maxdraws,max_records=maxrecords,cases_with_recoils=withkicks,invalid_cases=invalid,failures=failures,settings=labels,cpp_diagnostics=stats,
 input_sha256=hashlib.sha256(payload.encode()).hexdigest(),relative_tolerance=2e-12,absolute_tolerance=2e-13,transfer_momentum_absolute_tolerance=2e-11,
 sources=json.loads((Path(__file__).parents[1]/'reference/medium-source.json').read_text()),
 kinematics_sources=json.loads((Path(__file__).parents[1]/'reference/kinematics-source.json').read_text()),
 executables={Path(exe).name:hashlib.sha256(Path(exe).read_bytes()).hexdigest() for exe in [cpp,reference]})
Path(output).write_text(json.dumps(report,indent=2)+'\n');print({k:v for k,v in report.items() if k not in ['sources','executables','settings','failures','cpp_diagnostics','kinematics_sources']})
if failures:print(failures[:20]);raise SystemExit(f'{len(failures)} coherent mismatches')
assert any(v[2]>0 for v in stats), 'failed-kick rollback was not exercised'
assert any(v[0]>v[1]+1 for v in stats), 'resolution rejection was not exercised'
# Nonfinite reference cases are retained explicitly, never counted as finite parity.
