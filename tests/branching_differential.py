import hashlib,json,math,random,subprocess,sys
from pathlib import Path
cpp,reference,output=sys.argv[1:]
commands=['CONFIG\n208 3 1 .4 .59 .17 .17 .54 6.199999809265137 0 2 .45 1\n'];labels=[]
r=random.Random(2603)
def tape():
 values=[r.uniform(1e-12,1-1e-12) for _ in range(1000)]
 return '1000\n'+' '.join(format(v,'.17g') for v in values)+'\n'
for nf,q0,lps,pt in [(3,1,.2,1),(5,1.5,.25,2)]:
 for mode in range(6):
  commands.append(f'BCONFIG\n{nf} {q0} {lps} {pt} {mode}\n')
  for c in ['QQ','GC']:
   for initial in [0,1]:
    for theta in ([-1,.03,1] if not initial else [-1]):
     for minimum,maximum,energy,max2 in [(0,20,100,100),(q0,20,100,10),(q0,q0+1e-6,100,100),(2,1,100,100),(1,10,.5,100),(0,50,10,5)]:
      for repetition in range(4):
       commands.append(f'MASS\n{c} {minimum} {maximum:.17g} {theta} {energy} {max2} {initial}\n'+tape());labels.append('initial-mass' if initial else 'final-mass')
  for c in ['QQ','GG','QG']:
   for ti,energy,eps in [(0,0,.001),(q0*q0,10,.001),(4*q0*q0,10,.001),(25,5,.1),(0,0,.5),(0,0,.6)]:
    for repetition in range(4):
     commands.append(f'FRACTION\n{c} {ti:.17g} {energy} {eps}\n'+tape());labels.append('fraction')
payload=''.join(commands)
def run(exe):
 result=subprocess.run([exe],input=payload,text=True,capture_output=True)
 if result.returncode:raise RuntimeError((exe,result.returncode,len(result.stdout.splitlines()),result.stderr[-1000:]))
 rows=[list(map(float,line.split())) for line in result.stdout.splitlines()]
 assert len(rows)==len(labels)
 return rows
actual,expected=run(cpp),run(reference);groups={};failures=[]
for i,(name,a,b) in enumerate(zip(labels,actual,expected)):
 assert len(a)==len(b)
 g=groups.setdefault(name,dict(cases=0,max_absolute_error=0.,max_relative_error=0.,max_draws=0,draw_mismatches=0))
 g['cases']+=1;g['max_draws']=max(g['max_draws'],int(a[-1]))
 if a[-1]!=b[-1]:g['draw_mismatches']+=1;failures.append(dict(index=i,reason='RNG draw mismatch',cpp=a[-1],fortran=b[-1]))
 if name!='fraction' and a[-2]!=b[-2]:failures.append(dict(index=i,reason='flavour branch mismatch'))
 for col,(x,y) in enumerate(zip(a[:-1],b[:-1])):
  error=abs(x-y);g['max_absolute_error']=max(g['max_absolute_error'],error);g['max_relative_error']=max(g['max_relative_error'],error/max(abs(y),1e-300))
  if not math.isfinite(x) or not math.isclose(x,y,rel_tol=2e-12,abs_tol=2e-13):failures.append(dict(index=i,column=col,group=name,cpp=x,fortran=y))
report=dict(scope='branch candidate and fraction RNG replay, not full showers',groups=groups,failures=failures,
 relative_tolerance=2e-12,absolute_tolerance=2e-13,input_sha256=hashlib.sha256(payload.encode()).hexdigest(),
 sources=json.loads((Path(__file__).parents[1]/'reference/medium-source.json').read_text()),
 executables={Path(exe).name:hashlib.sha256(Path(exe).read_bytes()).hexdigest() for exe in [cpp,reference]})
Path(output).write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(groups,indent=2))
if failures:print(failures[:10]);raise SystemExit('Branching parity failed')
print('PASS',len(labels),'replay cases')
