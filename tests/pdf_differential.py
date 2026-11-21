import hashlib,json,math,subprocess,sys
from pathlib import Path
cpp,reference,output=sys.argv[1:]
commands=[];labels=[]
for nf,q0,lam,emax,scale,boost in [(3,1,.2,50,1,0),(5,1.5,.25,200,1,1),(0,.8,.15,20,2,0),(3,1,.2,6500,1,1)]:
 commands.append(f'CONFIG\n208 {nf} 1 .4 .59 .17 .17 .54 6.199999809265137 3 2 .45 1\nPCONFIG\n{nf} {q0} {lam} {emax} {scale} {boost}\nPTABLE\n');labels+=['table']*1000
 for c in ['QQ','QG','GG','GQ']:
  for q in [q0*.5,q0,q0*(1+1e-10),2*q0,10,20]:
   for z in [0,1e-12,1e-8,.001,.1,.5,.9,.999,1-1e-12,1]:
    commands.append(f'PDF\n{c} {z:.17g} {q:.17g}\n');labels.append('density')
  for q in [q0,q0*(1+1e-10),2*q0,10,20]:
   commands.append(f'PEXACT\n{c} {q:.17g} 0\n');labels.append('exact-integral')
  for q in [.5*q0,q0,2*q0,10,100,1000]:
   commands.append(f'PINT\n{c} {q:.17g} 0\n');labels.append('table-lookup')
payload=''.join(commands)
def run(exe):
 r=subprocess.run([exe],input=payload,text=True,capture_output=True)
 if r.returncode:raise RuntimeError((exe,r.returncode,len(r.stdout.splitlines()),r.stderr[-2000:]))
 rows=[list(map(float,line.split())) for line in r.stdout.splitlines()]
 if len(rows)!=len(labels):raise AssertionError((exe,len(rows),len(labels)))
 return rows
actual,expected=run(cpp),run(reference);groups={};failures=[]
for i,(name,a,b) in enumerate(zip(labels,actual,expected)):
 assert len(a)==len(b)
 g=groups.setdefault(name,dict(values=0,max_absolute_error=0.,max_relative_error=0.,outside_tolerance=0))
 for col,(x,y) in enumerate(zip(a,b)):
  error=abs(x-y);g['values']+=1;g['max_absolute_error']=max(g['max_absolute_error'],error);g['max_relative_error']=max(g['max_relative_error'],error/max(abs(y),1e-300))
  if not math.isfinite(x) or not math.isclose(x,y,rel_tol=2e-12,abs_tol=2e-13):
   g['outside_tolerance']+=1
   if len(failures)<30:failures.append(dict(row=i,column=col,group=name,cpp=x,fortran=y))
report=dict(scope='parton-in-parton PDF tables and integrals, not beam PDFs or full events',groups=groups,failures=failures,
 relative_tolerance=2e-12,absolute_tolerance=2e-13,input_sha256=hashlib.sha256(payload.encode()).hexdigest(),
 sources=json.loads((Path(__file__).parents[1]/'reference/medium-source.json').read_text()),
 executables={Path(exe).name:hashlib.sha256(Path(exe).read_bytes()).hexdigest() for exe in [cpp,reference]})
Path(output).write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(groups,indent=2))
if failures:print(failures[:10]);raise SystemExit('PDF parity failed')
print('PASS',sum(g['values'] for g in groups.values()),'comparisons')

# Deliberately unsafe scale: stock returns NaN, C++ reports a zero denominator.
unsafe='CONFIG\n208 3 1 .4 .59 .17 .17 .54 6.199999809265137 3 2 .45 1\nPCONFIG\n3 1 .2 50 1 0\nPDF\nQQ 0 100\n'
a=subprocess.run([cpp],input=unsafe,text=True,capture_output=True,timeout=10)
b=subprocess.run([reference],input=unsafe,text=True,capture_output=True,timeout=10)
assert a.returncode==2 and 'zero Sudakov denominator' in a.stderr
assert b.returncode==0 and math.isnan(float(b.stdout.strip()))
report['undefined_domain_probe']={'stock':'NaN','cpp':'zero Sudakov denominator exception','configuration':'NF=3 Q0=1 Lambda=.2 EMAX=50 boost=off, QQ x=0 Q=100'}
Path(output).write_text(json.dumps(report,indent=2)+'\n')
