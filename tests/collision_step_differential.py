import hashlib,json,math,random,subprocess,sys,itertools,collections
from pathlib import Path
cpp,reference,output=sys.argv[1:]
base='CONFIG\n208 3 1 .4 .59 .17 .17 .54 6.199999809265137 3 2 .45 1\nBCONFIG\n3 1 .2 1 0\n'
def row(x):return ' '.join(format(v,'.17g') for v in x)+'\n'
commands=[];inputs=[]
for mode,recmode,pdg,variant in itertools.product(range(4),range(4),[1,-1,21],range(16)):
 mass=2;energy=20;norm=math.sqrt(energy*energy-mass*mass)
 target=mass if variant%2 else mass+2;delta=target-mass;t=[-.2,-4,0,-200][variant%4]
 p=[.3*norm,.4*norm,math.sqrt(.75)*norm,energy]
 last=variant%2;status=2 if variant%3==0 else 1
 colours=[10 if pdg>0 else 0,11 if pdg<0 or pdg==21 else 0]
 softcut=100 if variant%8>=4 else 0;had=int(variant!=1);scat=int(variant!=2)
 z=0 if variant%3==0 else .4
 q=[.2,-.3,.4,math.sqrt(1+.04+.09+.16)]
 rng=random.Random(67100+len(inputs));tape=[rng.uniform(.000001,.999999) for _ in range(3000)]
 payload='HIT\n'+row([mode,1,1,t*(2-last),delta,target,delta,last])+row([*p,mass,pdg,status,0,0,0,.4,.1,.2,.3,.4])+row([-.2,.4,.6,1])
 payload+=row([t,21,*q,1,.1,.2,.3,1.2])+row([1,softcut,0 if variant>=8 else 2,had,scat,recmode,1,1])+row([z,1,7,3,1.8,1.21,variant%2])+row([*colours,20,variant%3,3])+'3000\n'+row(tape)
 commands.append(payload);inputs.append(dict(mode=mode,recmode=recmode,pdg=pdg,variant=variant,colours=colours,initial_used=variant%3))
payload=base+''.join(commands)
def run(exe):
 r=subprocess.run([exe],input=payload,text=True,capture_output=True)
 if r.returncode:raise RuntimeError((exe,r.returncode,r.stderr[-2500:]))
 rows=[list(map(float,line.split())) for line in r.stdout.splitlines()]
 assert len(rows)==9*len(inputs),(len(rows),len(inputs))
 return [rows[i:i+9] for i in range(0,len(rows),9)]
a,b=run(cpp),run(reference);failures=[];invalid=[];scalars=0;maximum=0;coverage=collections.Counter();maxdraws=0;cancellation=[]
for index,(info,actual,expected) in enumerate(zip(inputs,a,b)):
 if expected[0][-1]:
  invalid.append(dict(case=index,input=info,cpp_rejected=bool(actual[0][-1])))
  assert actual[0][-1]==1 and actual[0][9]==expected[0][9],(index,'nonfinite source rejection/draw mismatch')
  continue
 assert actual[0][-1]==0,(index,'unexpected C++ rejection')
 maxdraws=max(maxdraws,actual[0][9]);coverage['rejected' if actual[0][6] else 'accepted']+=1
 if actual[0][4]:coverage['split_recoil']+=1
 for i,(ar,br) in enumerate(zip(actual,expected)):
  assert len(ar)==len(br),(index,i,len(ar),len(br))
  for col,(x,y) in enumerate(zip(ar,br)):
   scalars+=1;maximum=max(maximum,abs(x-y))
   discrete=(i==0 and col>=4) or (1<=i<=4 and (col<2 or col in [11,12,20,21])) or i==5 or (i>=6 and col==0)
   absolute=2e-20 if i>=6 and col>=6 else 2e-13
   equal=x==y if discrete else math.isclose(x,y,rel_tol=2e-12,abs_tol=absolute)
   if i>=6 and col==10 and not equal and math.isfinite(x) and math.isfinite(y):
    # Near-lightlike dummy mass is a square root of cancelling terms.
    # Compare signed mass-squared with a forward-error bound from its input
    # components, retaining the raw mass difference in the report.
    norm=sum(v*v for v in br[6:10])
    propagated=sum(2*abs(v)*abs(v-w)+(v-w)**2 for v,w in zip(br[6:10],ar[6:10]))
    bound=16*sys.float_info.epsilon*norm+propagated
    residual=abs(math.copysign(x*x,x)-math.copysign(y*y,y))
    equal=residual<=bound
    cancellation.append(dict(case=index,row=i,cpp=x,fortran=y,signed_mass_squared_error=residual,roundoff_bound=bound,within_bound=equal))
   if not math.isfinite(x) or not equal:
    failures.append(dict(case=index,input=info,row=i,column=col,cpp=x,fortran=y))
 if actual[0][6]:
  assert actual[5]==[20,info['initial_used']]
 else:
  def balance(records):
   counts=collections.Counter()
   for colour,anti in records:
    if colour:counts[colour]+=1
    if anti:counts[anti]-=1
   return {c:n for c,n in counts.items() if n}
  assert balance([actual[3][11:13],actual[4][11:13]])==balance([info['colours']]),(index,'colour balance')
  assert actual[3][13:17]==actual[4][13:17] and actual[4][16]==1.2
  assert actual[2][17]==1.21
  coverage['recoil_status_'+str(int(actual[3][0]))]+=1
report=dict(scope='DOKINEMATICS one-transfer momentum/colour/timing/storage step; no outer sequence retry',cases=len(inputs),finite_cases=len(inputs)-len(invalid),scalars=scalars,max_absolute_error=maximum,max_draws=maxdraws,relative_tolerance=2e-12,absolute_tolerance=2e-13,dummy_absolute_tolerance=2e-20,coverage=dict(coverage),nonfinite_reference=invalid,dummy_mass_cancellation=cancellation,failures=failures,input_sha256=hashlib.sha256(payload.encode()).hexdigest(),sources={name:json.loads((Path(__file__).parents[1]/'reference'/name).read_text()) for name in ['collision-source.json','medium-source.json','kinematics-source.json']},executables={Path(e).name:hashlib.sha256(Path(e).read_bytes()).hexdigest() for e in [cpp,reference]})
Path(output).write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items() if k not in ['sources','executables','failures']},indent=2));print(failures[:10])
if failures:raise SystemExit('Collision step parity failed')
