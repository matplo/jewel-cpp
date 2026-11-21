import array,hashlib,json,math,subprocess,sys,tempfile,time
from pathlib import Path
cpp,reference,output=sys.argv[1:]
groups={};failures=[];records=[]
def compare(name,a,b):
 assert len(a)==len(b)
 g=groups.setdefault(name,dict(values=0,max_absolute_error=0.,max_relative_error=0.,outside_tolerance=0))
 for i,(x,y) in enumerate(zip(a,b)):
  error=abs(x-y);g['values']+=1;g['max_absolute_error']=max(g['max_absolute_error'],error);g['max_relative_error']=max(g['max_relative_error'],error/max(abs(y),1e-300))
  if not math.isfinite(x) or not math.isclose(x,y,rel_tol=2e-12,abs_tol=2e-13):
   g['outside_tolerance']+=1
   if len(failures)<30:failures.append(dict(group=name,index=i,cpp=x,fortran=y))
with tempfile.TemporaryDirectory() as tmp:
 for nf,q0,lps,lqcd,emax,scale,boost,floor in [(3,1,.2,.2,20,1,0,.45),(5,1.5,.25,.18,50,1.2,1,.45),(3,1,.2,.2,20,1,0,2.)]:
  base=f'CONFIG\n208 {nf} 1 .4 .59 .17 .17 .54 6.199999809265137 3 2 {floor} 1\nXCONFIG\n{nf} {q0} {lps} {lqcd} {emax} {scale} {boost}\n'
  queries=[]
  for c in ['QA','QB','GA','GB']:
   for t in [.5,1,2,10,100,1000]:
    for md in [.45,.51,.8,1.5,2]:queries.append(f'XINT\n{c} {t} {md}\n')
  for incoming in ['Q','G']:
   for outgoing in ['Q','G','C']:
    for mode in [0,1,2]:
     for momentum in [2,10,30]:
      for z,t in [(0,1),(math.sinh(1),math.cosh(1)),(-math.sinh(1),math.cosh(1))]:
       mass=.3;energy=math.sqrt(momentum*momentum+mass*mass)
       queries.append(f'XSEC\n{momentum} 0 0 {energy:.17g} {mass} 0 {incoming} {outgoing} 0 0 {z:.17g} {t:.17g} {mode}\n')
  queries.append('XSEC\n10 0 0 10 0 1000 Q C 0 0 0 1 0\n')
  queries.append('XSEC\n10 0 0 10 0 0 Q C 0 0 0 0 0\n')
  values=[];data=[]
  for exe in [cpp,reference]:
   dump=Path(tmp)/(Path(exe).name+'.bin');payload=base+'XDUMP\n'+str(dump)+'\n'+''.join(queries)
   start=time.monotonic();r=subprocess.run([exe],input=payload,text=True,capture_output=True)
   if r.returncode:raise RuntimeError((exe,r.returncode,r.stderr[-1500:]))
   raw=dump.read_bytes();a=array.array('d');a.frombytes(raw);assert len(a)==401100
   data.append(a);values.append([float(line) for line in r.stdout.splitlines()])
   records.append(dict(executable=Path(exe).name,nf=nf,q0=q0,lambda_shower=lps,lambda_scattering=lqcd,emax=emax,scale=scale,boost=boost,debye_floor=floor,seconds=time.monotonic()-start,table_sha256=hashlib.sha256(raw).hexdigest()))
   print('built',Path(exe).name,'NF',nf,flush=True)
  compare('full-grids',data[0],data[1]);compare('integrals-and-cross-sections',values[0],values[1])
report=dict(scope='scattering grids and channel cross sections, not transport events',groups=groups,failures=failures,records=records,
 relative_tolerance=2e-12,absolute_tolerance=2e-13,
 sources=json.loads((Path(__file__).parents[1]/'reference/medium-source.json').read_text()),
 executables={Path(exe).name:hashlib.sha256(Path(exe).read_bytes()).hexdigest() for exe in [cpp,reference]})
Path(output).write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(groups,indent=2))
if failures:print(failures[:10]);raise SystemExit('Scattering parity failed')
print('PASS',sum(g['values'] for g in groups.values()),'comparisons')
