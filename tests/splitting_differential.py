import collections,hashlib,json,math,random,subprocess,sys
from pathlib import Path
cpp,reference,output=sys.argv[1:]
def rotate(p,theta,phi):
 x,y,z,e=p;ct,st,cp,sp=math.cos(theta),math.sin(theta),math.cos(phi),math.sin(phi)
 return [ct*cp*x-sp*y+st*cp*z,ct*sp*x+cp*y+st*sp*z,-st*x+ct*z,e]
def record(status,pdg,parent,children,p,mass,fraction,angle,decay,theta,phi,colour,anti,qq=0):
 return [status,pdg,parent,*children,*rotate(p,theta,phi),mass,.1,.2,.3,.4,0,0,0,.4,decay,.25,fraction,angle,qq,1,colour,anti]
commands=[];labels=[];inputs=[];targets=[]
def fixture(mode,pdg,qq,z,cold,reject=False,skip=False,energy=100.):
 index=len(labels);theta,phi=(.7,-1.2) if index%2 else (0,0)
 e=energy;mass=10.;share=.4;mb=2.;mc=3.;pm=math.sqrt(e*e-mass*mass)
 pz=(2*share*e*e-mass*mass-mb*mb+mc*mc)/(2*pm);pt=math.sqrt(share*share*e*e-pz*pz-mb*mb)
 colour,anti=(101,102) if pdg==21 else (101,0) if pdg>0 else (0,102)
 rows=[record(11,21,0,[2,3],[0,0,pm,e],mass,share,.01 if reject else -1,1,theta,phi,101,102),
       record(11 if skip else (1 if index%2 else 2),pdg,1,[0,0],[-pt,0,pm-pz,(1-share)*e],mc,z,.3,2,theta,phi,colour,anti,qq),
       record(1,21,1,[0,0],[pt,0,pz,share*e],mb,.3,.3,3,theta,phi,101,102)]
 if cold:rows[1][14]=30
 rng=random.Random(7600+index);tape=[rng.uniform(.000001,.999999) for _ in range(3000)]
 ft=1 if index%2 else 2
 commands.append(f'BCONFIG\n3 1 .2 1 {mode}\nSPLIT\n3 2 {ft} 102\n'+'\n'.join(' '.join(format(x,'.17g') for x in row) for row in rows)+'\n3000\n'+' '.join(format(x,'.17g') for x in tape)+'\n')
 labels.append([mode,pdg,qq,z,cold,reject,skip,energy]);inputs.append(rows);targets.append(2)
for mode in range(6):
 for pdg,qq in [(1,0),(-1,0),(21,0),(21,1)]:
  for z in [.05,.4,.95]:
   for cold in [False,True]:fixture(mode,pdg,qq,z,cold)
# Boosted and asymmetric daughters amplify rounding in the subtraction of
# longitudinal momentum squared. Use common records and tapes in both engines.
for energy in [20.,1000.,10000.]:
 for mode in range(6):
  for z in [.05,.95]:fixture(mode,21,0,z,False,energy=energy)
for mode in [1,5]:fixture(mode,21,0,.4,False,True)
fixture(0,21,0,1e-7,False)
fixture(0,21,0,0,False)
fixture(0,21,0,.4,False,skip=True)
payload='CONFIG\n208 3 1 .4 .59 .17 .17 .54 6.199999809265137 0 2 .45 1\n'+''.join(commands)
def run(exe):
 result=subprocess.run([exe],input=payload,text=True,capture_output=True)
 if result.returncode:raise RuntimeError((exe,result.returncode,result.stderr[-3000:]))
 rows=[list(map(float,line.split())) for line in result.stdout.splitlines()];events=[];offset=0
 while offset<len(rows):
  header=rows[offset];assert len(header)==4;offset+=1;n=int(header[2]);events.append((header,rows[offset:offset+n]));offset+=n
 assert offset==len(rows)
 return events
actual,expected=run(cpp),run(reference)
# Freeze selected first-generation Fortran records as common inputs to both
# implementations, then split their massive daughters. This is a second
# generation check, not an end-to-end shower scheduler.
seen=set()
for label,(header,rows) in zip(list(labels),expected):
 mode=label[0]
 if header[0] or len(rows)!=5 or mode in seen:continue
 candidates=[i for i in [3,4] if rows[i][9]>0 and rows[i][20]>0]
 if not candidates:continue
 target=candidates[0]+1;seen.add(mode)
 rng=random.Random(8500+mode);tape=[rng.uniform(.000001,.999999) for _ in range(3000)]
 commands.append(f'BCONFIG\n3 1 .2 1 {mode}\nSPLIT\n5 {target} 1 {int(header[3])}\n'+'\n'.join(' '.join(format(x,'.17g') for x in row) for row in rows)+'\n3000\n'+' '.join(format(x,'.17g') for x in tape)+'\n')
 labels.append([mode,'second-generation',target]);inputs.append(rows);targets.append(target)
assert len(seen)==6
payload='CONFIG\n208 3 1 .4 .59 .17 .17 .54 6.199999809265137 0 2 .45 1\n'+''.join(commands)
actual,expected=run(cpp),run(reference)
assert len(actual)==len(expected)==len(labels)
failures=[];max_error=0.;max_relative=0.;draws=0;discarded=0;scalars=0;created=0
for label,original,target,(ha,a),(hb,b) in zip(labels,inputs,targets,actual,expected):
 if ha!=hb:failures.append(dict(case=label,header_cpp=ha,header_fortran=hb))
 assert len(a)==len(b);draws=max(draws,int(hb[1]));discarded+=int(hb[0])
 if not ha[0] and len(a)==len(original)+2:
  created+=1;parent,first,second=a[target-1],a[-2],a[-1]
  for col in range(5,9):assert math.isclose(first[col]+second[col],parent[col],rel_tol=1e-12,abs_tol=1e-12)
  for child in [first,second]:
   invariant=child[8]**2-sum(child[col]**2 for col in [5,6,7])
   # Daughter momentum comes from subtracting parent-scale squared terms.
   # Bound cancellation in this independent mass-shell check at that scale;
   # the direct C++/Fortran scalar tolerances below are unchanged.
   assert math.isclose(invariant,child[9]**2,rel_tol=1e-10,abs_tol=max(5e-11,32*math.ulp(parent[8]**2))), (label,invariant,child[9]**2)
  flow=collections.Counter()
  for child in [first,second]:
   flow[child[24]]+=1;flow[child[25]]-=1
  flow[parent[24]]-=1;flow[parent[25]]+=1
  assert all(value==0 for colour,value in flow.items() if colour!=0)
 for i,(rowa,rowb) in enumerate(zip(a,b)):
  assert len(rowa)==len(rowb)==26
  for col,(x,y) in enumerate(zip(rowa,rowb)):
   scalars+=1;error=abs(x-y);max_error=max(max_error,error);max_relative=max(max_relative,error/max(abs(y),1e-300))
   equal=x==y if col<5 or col>=22 else math.isclose(x,y,rel_tol=2e-12,abs_tol=2e-13)
   if not math.isfinite(x) or not equal:failures.append(dict(case=label,particle=i,column=col,cpp=x,fortran=y))
report=dict(scope='MAKESPLITTING event construction and angular rejection',cases=len(labels),created=created,scalars=scalars,max_absolute_error=max_error,max_relative_error=max_relative,max_draws=draws,discarded=discarded,failures=failures,input_sha256=hashlib.sha256(payload.encode()).hexdigest(),relative_tolerance=2e-12,absolute_tolerance=2e-13,sources={name:json.loads((Path(__file__).parents[1]/'reference'/name).read_text()) for name in ['medium-source.json','kinematics-source.json']},executables={Path(exe).name:hashlib.sha256(Path(exe).read_bytes()).hexdigest() for exe in [cpp,reference]})
Path(output).write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({k:v for k,v in report.items() if k not in ['sources','executables','failures']},indent=2))
if failures:print(failures[:10]);raise SystemExit('Splitting parity failed')
