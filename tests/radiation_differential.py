import hashlib,json,math,subprocess,sys
from pathlib import Path
cpp,reference,output=sys.argv[1:]
commands=[];labels=[]
for nf,q0,lam,emax,scale,boost in [(3,1,.2,50,1,0),(5,1.5,.25,100,1,1),(0,.8,.15,20,2,0),(3,1,.2,6500,1,1),(3,1,.2,1.5*150*math.cosh(2),1,0)]:
 commands.append(f'CONFIG\n208 {nf} 1 .4 .59 .17 .17 .54 6.199999809265137 3 2 .45 1\n')
 commands.append(f'RCONFIG\n{nf} {q0} {lam} {emax} {scale} {boost}\nRTABLE\n');labels+=['table']*1000
 for c in ['QQ','QG','GG','GC','GQ']:
  for q in [q0*.3,q0*.5,q0*.7,q0,q0*(1+1e-12),2,5,20,100]:
   commands.append(f'RSPLIT\n{c} {q:.17g} 0\n');labels.append('splitting')
  for low,high in [(q0,q0),(q0,2*q0),(2*q0,q0),(q0,20),(2*q0,30)]:
   commands.append(f'RSUDA\n{c} {low:.17g} {high:.17g}\n');labels.append('direct-sudakov')
  if c=='GQ':continue
  for q in [.5*q0,q0,1.1*q0,2*q0,10,30,500]:
   commands.append(f'RRED\n{c} {q:.17g} 0\n');labels.append('interpolation')
  for low,high in [(.5*q0,2*q0),(q0,q0),(2*q0,q0),(2*q0,10),(q0,500)]:
   commands.append(f'RFAST\n{c} {low:.17g} {high:.17g}\n');labels.append('ratio')
payload=''.join(commands)
def run(exe):
 r=subprocess.run([exe],input=payload,text=True,capture_output=True,check=True)
 values=[list(map(float,line.split())) for line in r.stdout.splitlines()]
 if len(values)!=len(labels):raise AssertionError((len(values),len(labels),r.stderr[:500]))
 return values
actual,expected=run(cpp),run(reference);groups={};failures=[]
for i,(name,a,b) in enumerate(zip(labels,actual,expected)):
 assert len(a)==len(b)
 group=groups.setdefault(name,dict(values=0,max_absolute_error=0.,max_relative_error=0.))
 for col,(x,y) in enumerate(zip(a,b)):
  error=abs(x-y);group['values']+=1;group['max_absolute_error']=max(group['max_absolute_error'],error);group['max_relative_error']=max(group['max_relative_error'],error/max(abs(y),1e-300))
  if not math.isfinite(x) or not math.isclose(x,y,rel_tol=2e-12,abs_tol=2e-13):failures.append(dict(row=i,column=col,group=name,cpp=x,fortran=y))
report=dict(scope='initial-state splitting and Sudakov tables, not full events',groups=groups,failures=failures,
 input_sha256=hashlib.sha256(payload.encode()).hexdigest(),relative_tolerance=2e-12,absolute_tolerance=2e-13,
 sources=json.loads((Path(__file__).parents[1]/'reference/medium-source.json').read_text()),
 executables={Path(exe).name:hashlib.sha256(Path(exe).read_bytes()).hexdigest() for exe in [cpp,reference]})
Path(output).write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(groups,indent=2))
if failures:print(failures[:10]);raise SystemExit(f'{len(failures)} differences')
print('PASS',sum(g['values'] for g in groups.values()),'comparisons')
