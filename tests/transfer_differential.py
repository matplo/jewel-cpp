import hashlib,json,math,random,subprocess,sys
from pathlib import Path
cpp,reference,output=sys.argv[1:]
groups={};failures=[];records=[];max_draws=0

def format_row(values):return ' '.join(format(x,'.17g') for x in values)+'\n'
def run(exe,payload):
 result=subprocess.run([exe],input=payload,text=True,capture_output=True)
 if result.returncode:raise RuntimeError((exe,result.returncode,result.stderr[-2500:]))
 rows=[list(map(float,line.split())) for line in result.stdout.splitlines()]
 assert len(rows)%6==0
 return [rows[i:i+6] for i in range(0,len(rows),6)]
def command(case,seed):
 slot,recalc,dt,x,leg,scratch,total,entry=case
 rng=random.Random(seed);tape=[rng.uniform(.000001,.999999) for _ in range(10000)]
 return 'TRANSFER\n'+format_row([slot,recalc,dt,x])+format_row(leg)+format_row(scratch)+format_row(total)+format_row(entry)+'10000\n'+format_row(tape)
for nf,boost in [(3,0),(3,1),(5,1)]:
 base=f'CONFIG\n208 {nf} 1 .4 .59 .17 .17 .54 6.199999809265137 3 2 .45 1\nQCONFIG\n{nf} .2 .2 {boost}\n'
 cases=[]
 for momentum in [2,10,100]:
  for mass in [0,.3,3]:
   for fraction in [.6,1]:
    energy=math.sqrt(momentum*momentum+mass*mass)
    if fraction*energy<=mass:continue
    for cold in [False,True]:
     for accumulated in [False,True]:
      direction=-1 if accumulated else 1
      leg=[.8*momentum,.4*momentum,direction*math.sqrt(.2)*momentum,energy,mass,20 if cold else .2,-.1,direction*.1,.4,.1,.2,.3,.4]
      total=[-.2,.1,-.3,-.1,-.13] if accumulated else [0,0,0,0,0]
      entry=[-1,0,0,0,0,7,21,0,0,0,.6,.6,0,0,0,1]
      cases.append([2 if accumulated else 1,0,.8,fraction,leg,[.4,.3,.2,.1],total,entry])
 # A null sum parallel to a much softer projectile makes every nonzero
 # collinear cold-medium transfer timelike: exercise the 1002-attempt fallback.
 cases.append([2,0,.8,1,[0,0,10,10,0,20,0,0,.4,.1,.2,.3,.4],
  [.4,.3,.2,.1],[0,0,1000,1000,0],[-1,0,0,0,0,7,21,0,0,0,.6,.6,0,0,0,1]])
 payload=base+''.join(command(case,11000+i) for i,case in enumerate(cases))
 actual,expected=run(cpp,payload),run(reference,payload)
 # Recalculate from independently sampled Fortran centres and transfer records.
 recalc=[]
 for i,(case,result) in enumerate(zip(cases,expected)):
  if i%8:continue
  entry=result[1]+result[2]
  recalc.append([1,1,.8,.35,case[4],case[5],[1,2,3,4,2],entry])
 recalc_payload=base+''.join(command(case,12000+i) for i,case in enumerate(recalc))
 actual+=run(cpp,recalc_payload);expected+=run(reference,recalc_payload);cases+=recalc
 assert len(actual)==len(expected)==len(cases)
 for index,(case,a,b) in enumerate(zip(cases,actual,expected)):
  name='retry-exhausted' if case[6][3]==1000 else 'recalculated' if case[1] else 'fresh-accumulated' if case[0]>1 else 'fresh'
  g=groups.setdefault(name,dict(cases=0,scalars=0,max_absolute_error=0.,max_relative_error=0.))
  g['cases']+=1
  if a[0]!=b[0]:failures.append(dict(config=[nf,boost],case=index,header_cpp=a[0],header_fortran=b[0]))
  max_draws=max(max_draws,int(b[0][1]))
  if name=='retry-exhausted':
   assert b[1][:5]==[0,0,0,0,0] and b[0][1]>=2004
  assert a[4][:5]==case[4][:5] # Projectile momentum and record mass restored exactly.
  assert a[3][4]<=0
  transfer=a[1][1:5];invariant=transfer[3]**2-sum(x*x for x in transfer[:3])
  assert math.isclose(invariant,a[1][0],rel_tol=1e-8,abs_tol=1e-8)
  for row,(ar,br) in enumerate(zip(a[1:],b[1:]),1):
   assert len(ar)==len(br)
   for col,(x,y) in enumerate(zip(ar,br)):
    g['scalars']+=1;error=abs(x-y);g['max_absolute_error']=max(g['max_absolute_error'],error);g['max_relative_error']=max(g['max_relative_error'],error/max(abs(y),1e-300))
    if not math.isfinite(x) or not math.isclose(x,y,rel_tol=2e-12,abs_tol=2e-13):failures.append(dict(config=[nf,boost],case=index,row=row,column=col,cpp=x,fortran=y))
 records.append(dict(nf=nf,boost=boost,cases=len(cases),input_sha256=hashlib.sha256((payload+recalc_payload).encode()).hexdigest()))
report=dict(scope='GETQVEC transfer sampling and accumulation, not full coherent scattering',groups=groups,max_draws=max_draws,failures=failures,records=records,relative_tolerance=2e-12,absolute_tolerance=2e-13,sources={name:json.loads((Path(__file__).parents[1]/'reference'/name).read_text()) for name in ['medium-source.json','kinematics-source.json']},executables={Path(exe).name:hashlib.sha256(Path(exe).read_bytes()).hexdigest() for exe in [cpp,reference]})
Path(output).write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(groups,indent=2));print('max draws',max_draws)
if failures:print(failures[:10]);raise SystemExit('Transfer parity failed')
