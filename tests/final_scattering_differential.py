import hashlib,json,math,random,subprocess,sys
from pathlib import Path
cpp,reference,output=sys.argv[1:]
groups={};failures=[];records=[];max_draws=0;max_active=0;changed_mass=0

def row(values):return ' '.join(format(x,'.17g') for x in values)+'\n'
def run(exe,payload):
 result=subprocess.run([exe],input=payload,text=True,capture_output=True)
 if result.returncode:raise RuntimeError((exe,result.returncode,result.stderr[-2500:]))
 lines=[list(map(float,line.split())) for line in result.stdout.splitlines()]
 assert len(lines)%104==0
 return [lines[i:i+104] for i in range(0,len(lines),104)]
for nf,boost,mode in [(3,0,0),(5,1,5)]:
 base=f'CONFIG\n208 {nf} 1 .4 .59 .17 .17 .54 6.199999809265137 3 2 .45 1\nFCONFIG\n{nf} 1 .2 100 1 {boost}\nRCONFIG\n{nf} 1 .2 100 1 {boost}\nXCONFIG\n{nf} 1 .2 .2 100 1 {boost}\nQCONFIG\n{nf} .2 .2 {boost}\nBCONFIG\n{nf} 1 .2 1 {mode}\n'
 commands=[];labels=[];input_masses=[];input_times=[]
 def append(pdg,mass,transfer,left,cross,formation,capacity=False):
  sign=-1 if boost else 1;energy=20;momentum=math.sqrt(energy*energy-mass*mass)
  after_energy=energy+transfer/2
  after_pz=(transfer-2*mass*mass+2*energy*after_energy)/(2*momentum)
  pt=math.sqrt(after_energy*after_energy-after_pz*after_pz-mass*mass)
  kick=[pt,0,sign*(after_pz-momentum),after_energy-energy]
  square=kick[3]**2-sum(x*x for x in kick[:3])
  entry=[transfer,*kick,1,21,0,0,0,1,1,0,0,0,1]
  leg=[0,0,sign*momentum,energy,mass,pdg,0,0,0,.4,.1,.2,.3,.4]
  count=100 if capacity else 1
  rng=random.Random(14000+len(labels));tape=[rng.uniform(.000001,.999999) for _ in range(20000)]
  if capacity:tape[:3]=[.5,.999999,.000001]
  payload=f'ECONFIG\n1 1 {formation} 20 {cross}\nFSTATE\n1 {left}\n'+row(leg)+row([0,.37,1,0])+row([1,count,*kick,square])+row(entry)
  payload+=row([0]*16)*(count-1)+'20000\n'+row(tape)
  commands.append(payload);labels.append('capacity' if capacity else 'final-state');input_masses.append(mass);input_times.append(left)
 for pdg in [1,21]:
  for mass in [0,2]:
   for transfer in [-.2,-4,-16]:
    for left,cross,formation in [(0,1,1),(.5,1,1),(5,1,2),(5,0,1),(2,20,.5)]:
     append(pdg,mass,transfer,left,cross,formation)
 append(21,2,-.2,2,20,1,True)
 payload=base+''.join(commands);actual,expected=run(cpp,payload),run(reference,payload)
 assert len(actual)==len(expected)==len(labels)
 for index,(label,mass,left,a,b) in enumerate(zip(labels,input_masses,input_times,actual,expected)):
  g=groups.setdefault(label,dict(cases=0,scalars=0,max_absolute_error=0.,max_relative_error=0.))
  g['cases']+=1;max_draws=max(max_draws,int(b[0][7]));max_active=max(max_active,int(b[0][6]));changed_mass+=b[0][0]!=mass
  assert 0<=a[0][1]<=left+2e-12 and 0<=a[0][0]<=20
  assert 1<=a[0][6]<=100 and -a[1][4]<=400*(1+1e-12)
  assert a[2][3]==20 and a[2][4]==mass
  if label=='capacity':assert b[0][7]==3 and a[0][7]==3
  for i,(ar,br) in enumerate(zip(a,b)):
   assert len(ar)==len(br)
   for col,(x,y) in enumerate(zip(ar,br)):
    g['scalars']+=1;error=abs(x-y);g['max_absolute_error']=max(g['max_absolute_error'],error);g['max_relative_error']=max(g['max_relative_error'],error/max(abs(y),1e-300))
    discrete=(i==0 and col>=3) or (i==3 and col<2) or (i>=4 and col==6)
    equal=x==y if discrete else math.isclose(x,y,rel_tol=2e-12,abs_tol=2e-13)
    if not math.isfinite(x) or not equal:failures.append(dict(config=[nf,boost,mode],case=index,row=i,column=col,cpp=x,fortran=y))
 records.append(dict(nf=nf,boost=boost,angular=mode,cases=len(labels),input_sha256=hashlib.sha256(payload.encode()).hexdigest()))
report=dict(scope='DOFISTATESCAT formation-time and transfer sequence, not collision application',groups=groups,max_draws=max_draws,max_active_transfers=max_active,changed_mass_cases=changed_mass,failures=failures,records=records,relative_tolerance=2e-12,absolute_tolerance=2e-13,sources={name:json.loads((Path(__file__).parents[1]/'reference'/name).read_text()) for name in ['medium-source.json','kinematics-source.json']},executables={Path(exe).name:hashlib.sha256(Path(exe).read_bytes()).hexdigest() for exe in [cpp,reference]})
Path(output).write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items() if k not in ['sources','executables','records','failures']},indent=2))
if failures:print(failures[:10]);raise SystemExit('Final scattering parity failed')
