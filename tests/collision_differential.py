import hashlib,json,math,random,subprocess,sys,itertools
from pathlib import Path
cpp,reference,output=sys.argv[1:]
base='CONFIG\n208 3 1 .4 .59 .17 .17 .54 6.199999809265137 3 2 .45 1\nBCONFIG\n3 1 .2 1 0\n'
commands=[];inputs=[]
def row(x):return ' '.join(format(v,'.17g') for v in x)+'\n'
for mode,mass,pdg,variation in itertools.product(range(4),[0,2,-2,-8],[1,21],range(12)):
 energy=20;norm=math.sqrt(energy**2-math.copysign(mass*mass,mass))
 vector=[.3*norm,.4*norm,math.sqrt(.75)*norm]
 target=[mass,0,2,8][variation%4];delta=target-mass
 t=[0,-.2,-4,-16,-80,-200][variation%6]
 q=[.2,-.3,.4] if variation%2 else [0,0,0];qm=1.;qe=math.sqrt(sum(v*v for v in q)+qm*qm)
 status=2 if variation%3==0 else 1;last=variation%2
 total=t*(1 if last else 2)
 leg=[*vector,energy,mass,pdg,status,0,0,0,.4,.1,.2,.3,.4]
 scatter=[t,21,*q,qe,qm,.1,.2,.3,1.2]
 rng=random.Random(2600+len(inputs));tape=[rng.uniform(.000001,.999999) for _ in range(3000)]
 command='PAIR\n'+row([mode,1,1,total,delta,target,delta,last])+row(leg)+row([-.2,.4,.6,1])+row(scatter)+'3000\n'+row(tape)
 commands.append(command);inputs.append(dict(mode=mode,mass=mass,pdg=pdg,variation=variation,p=leg[:4],q=[*q,qe]))
payload=base+''.join(commands)
def run(exe):
 r=subprocess.run([exe],input=payload,text=True,capture_output=True)
 if r.returncode:raise RuntimeError((exe,r.returncode,r.stderr[-3000:]))
 rows=[list(map(float,l.split())) for l in r.stdout.splitlines()]
 assert len(rows)==5*len(inputs)
 return [rows[i:i+5] for i in range(0,len(rows),5)]
a,b=run(cpp),run(reference);failures=[];maximum=0.;scalars=0;coverage={};maxdraws=0;nonfinite_reference=[];conservation_residuals=[]
for index,(info,actual,expected) in enumerate(zip(inputs,a,b)):
 if expected[0][-1]:
  nonfinite_reference.append(dict(case=index,input=info,cpp_rejected=bool(actual[0][-1])))
  assert actual[0][-1]==1 and actual[0][9]==expected[0][9],(index,'nonfinite reference not rejected')
  continue
 assert actual[0][-1]==0,(index,'unexpected C++ rejection')
 for i,(ar,br) in enumerate(zip(actual,expected)):
  assert len(ar)==len(br)
  for col,(x,y) in enumerate(zip(ar,br)):
   scalars+=1;maximum=max(maximum,abs(x-y));discrete=(i==0 and col>=4) or (i>0 and col<2)
   if not math.isfinite(x) or (x!=y if discrete else not math.isclose(x,y,rel_tol=2e-12,abs_tol=2e-13)):
    failures.append(dict(case=index,input=info,row=i,col=col,cpp=x,fortran=y))
 key=('rejected' if expected[0][6] else 'accepted')+('-cms' if expected[0][7] else '-rest')
 coverage[key]=coverage.get(key,0)+1;maxdraws=max(maxdraws,expected[0][9])
 if expected[0][4]:coverage['split-recoil']=coverage.get('split-recoil',0)+1
 if not actual[0][6]:
  residual=[actual[3][j+2]+actual[4][j+2]-info['p'][j]-info['q'][j] for j in range(4)]
  reference_residual=[expected[3][j+2]+expected[4][j+2]-info['p'][j]-info['q'][j] for j in range(4)]
  assert max(map(abs,residual))<1e-6,(index,"momentum conservation beyond source diagnostic threshold")
  if max(map(abs,residual))>1e-10:conservation_residuals.append(dict(case=index,cpp=residual,fortran=reference_residual))
report=dict(scope='DOKINEMATICS one-transfer momentum section, not complete routine',cases=len(inputs),finite_cases=len(inputs)-len(nonfinite_reference),relative_tolerance=2e-12,absolute_tolerance=2e-13,conservation_tolerance=1e-6,scalars=scalars,max_absolute_error=maximum,max_draws=maxdraws,nonfinite_reference=nonfinite_reference,conservation_residuals=conservation_residuals,coverage=coverage,failures=failures,input_sha256=hashlib.sha256(payload.encode()).hexdigest(),sources={name:json.loads((Path(__file__).parents[1]/'reference'/name).read_text()) for name in ['collision-source.json','medium-source.json','kinematics-source.json']},executables={Path(e).name:hashlib.sha256(Path(e).read_bytes()).hexdigest() for e in [cpp,reference]})
Path(output).write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items() if k not in ['sources','executables','failures']},indent=2));print(failures[:12])
if failures:raise SystemExit('Collision parity failed')
