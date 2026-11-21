import collections,hashlib,itertools,json,math,random,subprocess,sys
from pathlib import Path
cpp,reference,output=sys.argv[1:]
base='CONFIG\n208 3 1 .4 .59 .17 .17 .54 6.199999809265137 3 2 .45 1\n'
def row(v):return ' '.join(format(x,'.17g') for x in v)+'\n'
commands=[];inputs=[]
for (pdg,typ),mass,virt,x,status in itertools.product([(1,1),(-1,-1),(21,21),(21,1),(21,-1),(1,21),(-1,21)],[0,2,2.3,8],[0,1,3.7,4,12],[.05,.4,.9],[1,2]):
 index=len(inputs);nf=3 if index%2 else 5;angular=0 if index%2 else 5
 energy=40.;norm=math.sqrt(energy*energy-mass*mass);p=[.3*norm,.4*norm,math.sqrt(.75)*norm,energy]
 col,anti=(10 if pdg>0 else 0),(11 if pdg<0 or pdg==21 else 0)
 record=[status,pdg,0,0,0,*p,mass,.1,.2,.3,.4,1,2,3,.4,7,.25,.4,.3,0,1,col,anti]
 tsum=-100 if index%3 else -4;ft=1 if index%2 else 2
 rng=random.Random(38710+index);tape=[rng.uniform(.000001,.999999) for _ in range(3000)]
 commands.append(f'BCONFIG\n{nf} 1 .2 1 {angular}\nISPLIT\n1 1 {ft} 20\n'+row([x,tsum,virt,typ,4,.7,1])+row(record)+'3000\n'+row(tape))
 inputs.append(dict(nf=nf,angular=angular,pdg=pdg,typ=typ,mass=mass,virtuality=virt,x=x,status=status,p=p,colours=[col,anti]))
payload=base+''.join(commands)
def run(exe):
 r=subprocess.run([exe],input=payload,text=True,capture_output=True,timeout=120)
 if r.returncode:raise RuntimeError((exe,r.returncode,r.stderr[-2000:]))
 rows=[list(map(float,l.split())) for l in r.stdout.splitlines()];result=[];cursor=0
 for _ in inputs:
  n=int(rows[cursor][2]);result.append(rows[cursor:cursor+n+1]);cursor+=n+1
 assert cursor==len(rows)
 return result
a,b=run(cpp),run(reference);failures=[];invalid=[];maximum=0.;scalars=0;coverage=collections.Counter();maxdraws=0
for index,(info,actual,expected) in enumerate(zip(inputs,a,b)):
 if expected[0][-1]:
  invalid.append(dict(case=index,input=info,cpp_rejected=bool(actual[0][-1])))
  assert actual[0][-1]==1 and actual[0][1]==expected[0][1],(index,'nonfinite source rejection/draw mismatch')
  continue
 if actual[0][-1]:failures.append(dict(case=index,input=info,reason='unexpected C++ rejection'));continue
 assert len(actual)==len(expected),(index,actual[0],expected[0])
 maxdraws=max(maxdraws,expected[0][1]);accepted=len(expected)>2;coverage['accepted' if accepted else 'rejected']+=1
 for i,(ar,br) in enumerate(zip(actual,expected)):
  assert len(ar)==len(br)
  for col,(x,y) in enumerate(zip(ar,br)):
   scalars+=1;maximum=max(maximum,abs(x-y));discrete=(i==0 and col!=4) or (i>0 and (col<5 or col>=22))
   if not math.isfinite(x) or (x!=y if discrete else not math.isclose(x,y,rel_tol=2e-12,abs_tol=2e-13)):
    failures.append(dict(case=index,input=info,row=i,col=col,cpp=x,fortran=y))
 if accepted:
  coverage['spacelike_continuing' if actual[3][9]<0 else 'timelike_or_massless_continuing']+=1
  if actual[2][9]>0:coverage['massive_emission']+=1
  for j in range(4):assert math.isclose(actual[2][5+j]+actual[3][5+j],info['p'][j],rel_tol=1e-12,abs_tol=1e-12),(index,'conservation')
  assert actual[2][14:18]==actual[3][14:18] and math.isclose(actual[2][17],3.3)
  def balance(records):
   c=collections.Counter()
   for colour,anti in records:
    if colour:c[colour]+=1
    if anti:c[anti]-=1
   return {k:v for k,v in c.items() if v}
  assert balance([actual[2][24:26],actual[3][24:26]])==balance([info['colours']]),(index,'colour balance')
 else:assert actual[0][4]==1
report=dict(scope='Full MAKEINSPLIT calls',cases=len(inputs),finite_cases=len(inputs)-len(invalid),scalars=scalars,max_absolute_error=maximum,max_draws=maxdraws,coverage=dict(coverage),nonfinite_reference=invalid,failures=failures,relative_tolerance=2e-12,absolute_tolerance=2e-13,input_sha256=hashlib.sha256(payload.encode()).hexdigest(),sources={name:json.loads((Path(__file__).parents[1]/'reference'/name).read_text()) for name in ['medium-source.json','kinematics-source.json']},executables={Path(e).name:hashlib.sha256(Path(e).read_bytes()).hexdigest() for e in [cpp,reference]})
Path(output).write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items() if k not in ['sources','executables','failures','nonfinite_reference']},indent=2));print('nonfinite',len(invalid));print(failures[:12])
if failures:raise SystemExit('Initial splitting parity failed')
