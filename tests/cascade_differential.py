"""Whole MAKEBRANCH and MAKECASCADE replay against frozen JEWEL."""
import hashlib,json,math,random,subprocess,sys
from pathlib import Path
baseline_only=sys.argv[-1]=='--baseline'
args=sys.argv[1:-1] if baseline_only else sys.argv[1:]
cpp,reference,output=args
def row(v):return ' '.join(format(x,'.17g') for x in v)+'\n'
base='CONFIG\n208 3 1 .4 .59 .17 .17 .54 6.199999809265137 0 2 .45 1\nFCONFIG\n3 1 .2 100 1 0\nRCONFIG\n3 1 .2 100 1 0\nXCONFIG\n3 1 .2 .2 100 1 0\nPCONFIG\n3 1 .2 100 1 0\nQCONFIG\n3 .2 .2 0\nBCONFIG\n3 1 .2 1 0\n'
cases=[];labels=[];capacity=64;active_nf=3;active_boost=0

def rec(status,pdg,parent,daughters,p,mass,colour,anti):
 return [status,pdg,parent,*daughters,*p,mass,0,0,0,0,0,0,0,.4,8,.4,.3,0,0,colour,anti,.5]
def append(pdg,first,second,direction,selected,left,cross,length,kinmode=1,history=0,recmode=0,subtract=0,energy=20,mass=4,rotation=0,shorten=False,weight=1,enabled=1,near_edge=False,retry=False,rejection=0,force_radiation=False,operation="BRANCH",status=1,start=.5,birth=.4,scatter_recoils=1):
 momentum=math.sqrt(energy*energy-mass*mass);z=.4;ma=3 if mass>=10 else .3;mb=2 if mass>=10 else .5
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
 records[2][18]=birth
 for r in records[3:]:
  r[0]=status;r[17]=birth;r[18]=max(.6,.5+left);r[25]=start
 if status==2:
  for r in records[3:]:r[19]=-1;r[9]=0;r[8]=math.sqrt(sum(v*v for v in r[5:8]))
 records[0]=[0]*26;records[0][19]=-1
 if shorten:records[4 if selected==4 else 3][18]=1.2
 seed=260000+len(cases);rng=random.Random(seed);tape_count=2000000 if operation=="CASCADE" and cross>=5 else 500000 if operation=="CASCADE" and cross else 100000
 tape=[rng.uniform(.000001,.999999) for _ in range(tape_count)]
 if force_radiation:tape[:4]=[.999999,.000001,.999999,.9]
 config=f'ECONFIG\n1 1 1 3 {cross}\n{operation}\n'+row([selected,enabled,1,left,.05,.4,length,kinmode,110,weight,history])
 config+=row([int(retry),rejection])
 config+=''.join(row(r) for r in records)+row([1,0,0,1,scatter_recoils,recmode,1,subtract,capacity])
 for _ in range(history):config+=row([-.2,.1,.2,.3,.01,.6,21,0,0,0,1,1,0,0,0,.6])
 config+=str(tape_count)+'\n'+row(tape)
 cases.append(config);labels.append(dict(pdg=pdg,selected=selected,left=left,cross=cross,length=length,seed=seed,kinmode=kinmode,history=history,recmode=recmode,subtract=subtract,energy=energy,mass=mass,rotation=rotation,shorten=shorten,weight=weight,enabled=enabled,near_edge=near_edge,nf=active_nf,boost=active_boost,retry=retry,rejection=rejection,force_radiation=force_radiation,selected_flavour=first if selected==4 else second,operation=operation,status=status,start=start,birth=birth,scatter_recoils=scatter_recoils))
for pdg,first,second in [(21,21,21),(1,21,1),(-1,21,-1)]:
 for selected in [4,5]:
  for left,cross,enabled in [(0,0,0),(.5,0,0),(.5,1,0),(.5,5,0),(.5,1,1)]:
   append(pdg,first,second,1,selected,left,cross,1,enabled=enabled)
for status in [1,2]:
 for start,birth in [(-1e6,.4),(-1e6,-.5),(-.2,-.5),(1,.4)]:
  append(21,21,21,1,4,.5,0,1,enabled=0,status=status,start=start,birth=birth)
for pdg,first,second in [(21,21,21),(1,21,1)]:
 for status in [1,2]:
  append(pdg,first,second,1,4,.5,0,1,enabled=0,status=status,operation="CASCADE",scatter_recoils=0)
if not baseline_only:
 for pdg,first,second in [(21,21,21),(1,21,1),(-1,21,-1)]:
  for cross,enabled in [(0,0),(1,0),(1,1),(5,1)]:
   append(pdg,first,second,1,4,.5,cross,1,enabled=enabled,energy=100,mass=10,operation="CASCADE",scatter_recoils=0)
 for kinmode in [0,1,2,3]:
  append(21,21,21,1,4,.5,1,1,enabled=0,energy=100,mass=10,rotation=.8,kinmode=kinmode,operation="CASCADE",scatter_recoils=0)
payload=base+''.join(cases)
def run(exe):
 try:
  r=subprocess.run([exe],input=payload,text=True,capture_output=True,timeout=240)
 except subprocess.TimeoutExpired as error:
  for suffix,data in [('out',error.stdout),('err',error.stderr)]:
   if isinstance(data,bytes):data=data.decode(errors='replace')
   Path(output).with_suffix('.'+Path(exe).name+'.'+suffix).write_text(data or '')
  raise
 Path(output).with_suffix('.'+Path(exe).name+'.out').write_text(r.stdout)
 Path(output).with_suffix('.'+Path(exe).name+'.err').write_text(r.stderr)
 if r.returncode:raise RuntimeError((exe,r.returncode,r.stderr[-3000:]))
 rows=iter(r.stdout.splitlines());out=[]
 for _ in cases:
  h=list(map(float,next(rows).split()));n=int(h[0]);out.append([h]+[list(map(float,next(rows).split())) for _ in range(n+103+capacity)])
 assert next(rows,None) is None
 return out
a,b=run(cpp),run(reference);failures=[];values=0;maximum=0.
for index,(actual,expected) in enumerate(zip(a,b)):
 if actual[0]!=expected[0] or expected[0][6]:failures.append(dict(case=index,header_cpp=actual[0],header_fortran=expected[0]));continue
 n=int(expected[0][0]);assert len(actual)==len(expected)
 for i,(ar,br) in enumerate(zip(actual,expected)):
  assert len(ar)==len(br)
  for j,(x,y) in enumerate(zip(ar,br)):
   values+=1;maximum=max(maximum,abs(x-y))
   discrete=i==0 or (1<=i<=n and (j<5 or 21<=j<=24 or j==26)) or (n+2<=i<=n+101 and j==6) or (n+102<=i<=n+101+capacity and j==0)
   if not math.isfinite(x) or not (x==y if discrete else math.isclose(x,y,rel_tol=2e-12,abs_tol=2e-13)):
    failures.append(dict(case=index,row=i,column=j,cpp=x,fortran=y))
report=dict(scope="MAKEBRANCH and MAKECASCADE",baseline_only=baseline_only,cases=len(cases),values=values,max_absolute_error=maximum,failures=failures,settings=labels,
 max_draws=max(int(r[0][4]) for r in b),max_records=max(int(r[0][0]) for r in b),input_sha256=hashlib.sha256(payload.encode()).hexdigest(),
 sources=json.loads((Path(__file__).parents[1]/'reference/medium-source.json').read_text()),
 executables={Path(exe).name:hashlib.sha256(Path(exe).read_bytes()).hexdigest() for exe in [cpp,reference]})
Path(output).write_text(json.dumps(report,indent=2)+'\n')
print({k:v for k,v in report.items() if k not in ['sources','executables','settings','failures']})
if failures:print(failures[:30]);raise SystemExit(f'{len(failures)} scheduler mismatches')
