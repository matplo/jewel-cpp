import hashlib,json,math,random,subprocess,sys,itertools,os
from pathlib import Path
cpp,reference,output=sys.argv[1:]
base='CONFIG\n208 3 1 .4 .59 .17 .17 .54 6.199999809265137 3 2 .45 1\nBCONFIG\n3 1 .2 1 0\n'
def row(x):return ' '.join(format(v,'.17g') for v in x)+'\n'
commands=[];inputs=[]
settings=[(*s,None) for s in itertools.product(range(4),[1,-1,21],[0,2,4,20],range(5),[1,.6])]
settings += [(mode,pdg,target,0,x,mass) for mode,pdg,mass,x in itertools.product(range(4),[1,21],[-2,-8],[1,.6]) for target in [mass,0,2]]
for mode,pdg,target,pattern,x,override_mass in settings:
 index=len(inputs);mass=(0 if pattern==4 else 2) if override_mass is None else override_mass;energy=20;norm=math.sqrt(energy*energy-math.copysign(mass*mass,mass))
 values=[[-.2,-.3],[-4,-2],[0,-.2,0],[-200,-.2],[0,0]][pattern]
 status=2 if pattern==1 else 14 if pattern==3 else 1
 p=[.3*norm,.4*norm,math.sqrt(.75)*norm,energy]
 colours=[10 if pdg>0 else 0,11 if pdg<0 or pdg==21 else 0]
 first=2 if pattern==2 else 1
 q=[.2,-.3,.4,math.sqrt(1+.04+.09+.16)]
 rng=random.Random(92000+index);tape=[rng.uniform(.000001,.999999) for _ in range(5000)]
 payload='SEQUENCE\n'+row([mode,1,1,target,3,x,.4,1,index%2,first,len(values)])
 payload+=row([*p,mass,pdg,status,0,0,0,.4,.1,.2,.3,.4,7,.4,.3,*colours,20])
 payload+=row([1,0,0,1,1,index%4,1,1,0,3])
 for j,t in enumerate(values):payload+=row([t,1.21+.3*j,21,*q,1,.1,.2,.3,1.2+.3*j])
 payload+='5000\n'+row(tape)
 commands.append(payload);inputs.append(dict(mode=mode,pdg=pdg,target=target,pattern=pattern,x=x,mass=mass))
payload=base+''.join(commands)
def run(exe,payload=payload,count=len(inputs)):
 r=subprocess.run([exe],input=payload,text=True,capture_output=True,timeout=120)
 if r.returncode:raise RuntimeError((exe,r.returncode,r.stderr[-2500:]))
 rows=[list(map(float,line.split())) for line in r.stdout.splitlines()];result=[];cursor=0
 for _ in range(count):
  n=int(rows[cursor][0]);result.append(rows[cursor:cursor+n+4]);cursor+=n+4
 assert cursor==len(rows)
 return result
a=run(cpp);exceptional={i for i,r in enumerate(a) if r[0][-1] in [2,3]};guarded=[]
for i in sorted(exceptional):
 try:
  r=subprocess.run([reference],input=base+commands[i],text=True,capture_output=True,timeout=1)
  assert r.returncode==6,(i,'C++ guarded a reference that completed',r.returncode)
  outcome='replay exhausted (STOP 6)'
 except subprocess.TimeoutExpired:outcome='did not complete within 1 second; terminated by harness'
 guarded.append(dict(case=i,input=inputs[i],cpp_guard=int(a[i][0][-1]),reference_outcome=outcome))
subset=[i for i in range(len(inputs)) if i not in exceptional]
reference_rows=run(reference,base+''.join(commands[i] for i in subset),len(subset))
b={i:r for i,r in zip(subset,reference_rows)};failures=[];invalid=[];scalars=0;maximum=0.;cancellation=[];stale=[];retry_count=0;max_records=0;maxdraws=0
for index in subset:
 info,actual,expected=inputs[index],a[index],b[index]
 if expected[0][-1]:
  invalid.append(dict(case=index,input=info,cpp_rejected=bool(actual[0][-1])))
  if actual[0][-1]==1:continue
  if actual[0][-1]==0 and any(not math.isfinite(v) for record in expected[1:1+int(expected[0][0])] for v in record):
   # Recovery is an explicit numerical extension, not a finite-source parity case.
   diagnostic=subprocess.run([cpp],input=base+commands[index],text=True,capture_output=True,check=True,timeout=120,env={**os.environ,'JEWEL_COLLISION_TRACE':'1'})
   repairs=[list(map(float,line.split()[1:])) for line in diagnostic.stderr.splitlines() if line.startswith('COLLISION_ZERO_P2 ')]
   assert repairs,(index,'recovered nonfinite reference without zero-CM repair')
   for residue,mass2,energy in repairs:
    assert residue<0 and mass2>0 and energy==math.sqrt(mass2)
    assert abs(residue)<=4*sys.float_info.epsilon*mass2
   assert all(math.isfinite(v) for record in actual for v in record)
   invalid[-1]['zero_cm_repairs']=repairs
   continue
 if actual[0][-1]:failures.append(dict(case=index,input=info,reason='unexpected C++ domain rejection'));continue
 if len(actual)!=len(expected):failures.append(dict(case=index,input=info,cpp_header=actual[0],fortran_header=expected[0]));continue
 n=int(expected[0][0]);max_records=max(max_records,n);maxdraws=max(maxdraws,expected[0][6]);retry_count+=int(expected[0][3])
 for i,(ar,br) in enumerate(zip(actual,expected)):
  assert len(ar)==len(br),(index,i,len(ar),len(br))
  for col,(v,w) in enumerate(zip(ar,br)):
   if 1<=i<=n and col in [3,4] and w>n:
    stale.append(dict(case=index,row=i,column=col,fortran=w));w=0
   if i==0 and col==8 and expected[0][-1]:continue
   if not math.isfinite(w):
    assert i>n and col==10 and math.isfinite(v),(index,'unexpected nonfinite reference field')
    norm=sum(t*t for t in br[6:10])
    bound=16*sys.float_info.epsilon*norm+sum(2*abs(t)*abs(t-u)+(t-u)**2 for t,u in zip(br[6:10],ar[6:10]))
    source_invariant=br[9]**2-sum(t*t for t in br[6:9])
    assert abs(math.copysign(v*v,v)-source_invariant)<=bound,(index,'nonfinite dummy beyond roundoff bound')
    continue
   scalars+=1;maximum=max(maximum,abs(v-w))
   discrete=(i==0 and col!=5) or (1<=i<=n and (col<5 or col>=21)) or (i>n and col==0)
   absolute=2e-20 if i>n and col>=6 else 2e-13
   equal=v==w if discrete else math.isclose(v,w,rel_tol=2e-12,abs_tol=absolute)
   if i>n and col==10 and not equal and math.isfinite(v) and math.isfinite(w):
    bound=16*sys.float_info.epsilon*sum(t*t for t in br[6:10])+sum(2*abs(t)*abs(t-u)+(t-u)**2 for t,u in zip(br[6:10],ar[6:10]))
    error=abs(math.copysign(v*v,v)-math.copysign(w*w,w));equal=error<=bound
    cancellation.append(dict(case=index,row=i,cpp=v,fortran=w,invariant_error=error,roundoff_bound=bound,within_bound=equal))
   if not math.isfinite(v) or not equal:failures.append(dict(case=index,input=info,row=i,column=col,cpp=v,fortran=w))
report=dict(scope='Full DOKINEMATICS calls with active event-record comparison',cases=len(inputs),finite_cases=len(inputs)-len(invalid)-len(guarded),guarded_reference=guarded,scalars=scalars,max_absolute_error=maximum,max_records=max_records,max_draws=maxdraws,retry_splitting_cases=retry_count,nonfinite_reference=invalid,cleared_inactive_daughter_links=stale,dummy_mass_cancellation=cancellation,failures=failures,input_sha256=hashlib.sha256(payload.encode()).hexdigest(),sources={name:json.loads((Path(__file__).parents[1]/'reference'/name).read_text()) for name in ['medium-source.json','kinematics-source.json']},executables={Path(e).name:hashlib.sha256(Path(e).read_bytes()).hexdigest() for e in [cpp,reference]})
Path(output).write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items() if k not in ['sources','executables','failures','nonfinite_reference','cleared_inactive_daughter_links','dummy_mass_cancellation','guarded_reference']},indent=2));print('guarded',len(guarded),'nonfinite',len(invalid),'stale',len(stale),'cancellation',len(cancellation));print(failures[:20])
if failures:raise SystemExit('Collision sequence parity failed')
