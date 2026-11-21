import hashlib,json,math,random,subprocess,sys
from pathlib import Path
cpp,reference,output=sys.argv[1:]
def rotate(p,theta,phi):
 x,y,z,e=p;ct,st,cp,sp=math.cos(theta),math.sin(theta),math.cos(phi),math.sin(phi)
 return [ct*cp*x-sp*y+st*cp*z,ct*sp*x+cp*y+st*sp*z,-st*x+ct*z,e]
def record(status,pdg,parent,children,p,mass,fraction,angle,decay,theta,phi):
 return [status,pdg,parent,*children,*rotate(p,theta,phi),mass,.1,.2,.3,.4,1,2,0,.4,decay,.25,fraction,angle,0,1]
commands=[];lengths=[];labels=[]
for mode in range(6):
 commands.append(f'BCONFIG\n3 1 .2 1 {mode}\n')
 for pdg1,pdg2 in [(21,1),(21,-1),(21,21),(-1,1)]:
  for angle1,angle2 in [(.3,.3),(.3,.01),(.01,.3),(.01,.01)]:
   for theta,phi in [(0,0),(.7,-1.2)]:
    for scattered in [False,True]:
     e=100.;mass=10.;z=.4;mb=2.;mc=3.;pm=math.sqrt(e*e-mass*mass)
     pz=(2*z*e*e-mass*mass-mb*mb+mc*mc)/(2*pm)
     pt=math.sqrt(z*z*e*e-pz*pz-mb*mb)
     rows=[record(11,21,0,[2,3],[0,0,pm,e],mass,z,.1,1,theta,phi),
           record(12 if scattered else 1,pdg1,1,[4,5] if scattered else [0,0],[-pt,0,pm-pz,(1-z)*e],mc,.3,angle1,2,theta,phi),
           record(1,pdg2,1,[0,0],[pt,0,pz,z*e],mb,.3,angle2,3,theta,phi)]
     if scattered:
      rows.extend([record(1,21,2,[0,0],[1,0,0,1],0,0,0,0,0,0),record(1,pdg1,2,[0,0],[2,0,0,2],0,0,0,0,0,0)])
     rng=random.Random(601+len(lengths));tape=[rng.uniform(.000001,.999999) for _ in range(3000)]
     commands.append(f'REJECT\n{len(rows)} 1\n'+'\n'.join(' '.join(format(x,'.17g') for x in row) for row in rows)+'\n3000\n'+' '.join(format(x,'.17g') for x in tape)+'\n')
     lengths.append(len(rows));labels.append([mode,pdg1,pdg2,angle1,angle2,theta,scattered])
# Defined failure path: even zero daughter masses cannot fit this fraction.
rows=[record(11,21,0,[2,3],[0,0,math.sqrt(9900),100],10,1e-5,.1,1,.7,-1.2),
      record(1,21,1,[0,0],[-2,0,60,61],3,.3,.3,2,.7,-1.2),
      record(1,1,1,[0,0],[2,0,39,40],2,.3,.3,3,.7,-1.2)]
rng=random.Random(999);tape=[rng.uniform(.000001,.999999) for _ in range(3000)]
commands.append('REJECT\n3 1\n'+'\n'.join(' '.join(format(x,'.17g') for x in row) for row in rows)+'\n3000\n'+' '.join(format(x,'.17g') for x in tape)+'\n')
lengths.append(3);labels.append(['discard'])
payload='CONFIG\n208 3 1 .4 .59 .17 .17 .54 6.199999809265137 0 2 .45 1\n'+''.join(commands)
def run(exe):
 result=subprocess.run([exe],input=payload,text=True,capture_output=True)
 if result.returncode:raise RuntimeError((exe,result.returncode,result.stderr[-2000:]))
 rows=[list(map(float,line.split())) for line in result.stdout.splitlines()]
 assert len(rows)==sum(n+1 for n in lengths)
 return rows
actual,expected=run(cpp),run(reference);offset=0;failures=[];max_error=0.;max_relative=0.;draws=0;discarded=0;scalars=0
for label,n in zip(labels,lengths):
 a,b=actual[offset],expected[offset];assert len(a)==len(b)==2
 if a!=b:failures.append(dict(case=label,header_cpp=a,header_fortran=b))
 draws=max(draws,int(b[1]));discarded+=int(b[0]);offset+=1
 if not a[0]:
  mother,first,second=actual[offset:offset+3]
  for col in range(5,9):
   assert math.isclose(first[col]+second[col],mother[col],rel_tol=1e-12,abs_tol=1e-12)
  for child in [first,second]:
   invariant=child[8]**2-sum(child[col]**2 for col in [5,6,7])
   assert math.isclose(invariant,child[9]**2,rel_tol=1e-10,abs_tol=5e-11)
 for i in range(n):
  a,b=actual[offset],expected[offset];offset+=1;assert len(a)==len(b)==24
  for col,(x,y) in enumerate(zip(a,b)):
   scalars+=1;error=abs(x-y);max_error=max(max_error,error);max_relative=max(max_relative,error/max(abs(y),1e-300))
   equal=x==y if col<5 or col>=22 else math.isclose(x,y,rel_tol=2e-12,abs_tol=2e-13)
   if not math.isfinite(x) or not equal:failures.append(dict(case=label,particle=i,column=col,cpp=x,fortran=y))
report=dict(scope='REJECTSPLITTING sibling event-record repair',cases=len(lengths),scalars=scalars,max_absolute_error=max_error,max_relative_error=max_relative,max_draws=draws,discarded=discarded,failures=failures,input_sha256=hashlib.sha256(payload.encode()).hexdigest(),relative_tolerance=2e-12,absolute_tolerance=2e-13,sources={name:json.loads((Path(__file__).parents[1]/'reference'/name).read_text()) for name in ['medium-source.json','kinematics-source.json']},executables={Path(exe).name:hashlib.sha256(Path(exe).read_bytes()).hexdigest() for exe in [cpp,reference]})
Path(output).write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({k:v for k,v in report.items() if k not in ['sources','executables','failures']},indent=2))
if failures:print(failures[:10]);raise SystemExit('Rejection parity failed')
