import array,hashlib,json,math,subprocess,sys,tempfile,time
from pathlib import Path
cpp,reference,output=sys.argv[1:]
groups={};failures=[];records=[]
def compare(name,actual,expected):
 if len(actual)!=len(expected):raise AssertionError((name,len(actual),len(expected)))
 g=groups.setdefault(name,dict(values=0,max_absolute_error=0.,max_relative_error=0.,outside_tolerance=0))
 for i,(x,y) in enumerate(zip(actual,expected)):
  error=abs(x-y);g['values']+=1;g['max_absolute_error']=max(g['max_absolute_error'],error);g['max_relative_error']=max(g['max_relative_error'],error/max(abs(y),1e-300))
  if not math.isfinite(x) or not math.isclose(x,y,rel_tol=2e-12,abs_tol=2e-13):
   g['outside_tolerance']+=1
   if len(failures)<30:failures.append(dict(group=name,index=i,cpp=x,fortran=y))
with tempfile.TemporaryDirectory() as tmp:
 for nf,q0,lam,emax,boost in [(3,1,.2,50,0),(5,1.5,.25,200,1),(3,1,.2,1.5*150*math.cosh(2),0)]:
  base=f'CONFIG\n208 {nf} 1 .4 .59 .17 .17 .54 6.199999809265137 0 2 .45 1\nFCONFIG\n{nf} {q0} {lam} {emax} 1 {boost}\n'
  queries=[]
  for c in ['QQ','QG','GG','GC']:
   for angular in range(6):
    for q in [q0,q0*(1+1e-10),2*q0,10,100,200]:
     for zeta in [.5,1]:queries.append(f'FSPLIT\n{c} 10000 {q:.17g} {zeta} 2000 {angular}\n')
    for high,low in [(q0,q0),(2*q0,q0),(10,q0),(20,2*q0),(q0,2*q0)]:
     queries.append(f'FSUDA\n{c} {high:.17g} 10000 {low:.17g} .5 2000 {angular}\n')
  values=[];data=[]
  for exe in [cpp,reference]:
   dump=Path(tmp)/(Path(exe).name+'.bin')
   payload=base+'FDUMP\n'+str(dump)+'\n'+''.join(queries)
   start=time.monotonic();r=subprocess.run([exe],input=payload,text=True,capture_output=True,check=True)
   raw=dump.read_bytes();a=array.array('d');a.frombytes(raw)
   if len(a)!=3002000:raise AssertionError(('wrong full table size',len(a)))
   values.append([float(line) for line in r.stdout.splitlines()]);data.append(a)
   records.append(dict(executable=Path(exe).name,nf=nf,q0=q0,lambda_shower=lam,emax=emax,boost=boost,seconds=time.monotonic()-start,table_sha256=hashlib.sha256(raw).hexdigest()))
   print('built',Path(exe).name,'NF',nf,flush=True)
  compare('full-tables',data[0],data[1]);compare('lookups-and-sudakov',values[0],values[1])
report=dict(scope='full final-state splitting tables and queries, not full events',groups=groups,failures=failures,records=records,
 relative_tolerance=2e-12,absolute_tolerance=2e-13,
 sources=json.loads((Path(__file__).parents[1]/'reference/medium-source.json').read_text()),
 executables={Path(exe).name:hashlib.sha256(Path(exe).read_bytes()).hexdigest() for exe in [cpp,reference]})
Path(output).write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(groups,indent=2))
if failures:print(failures[:10]);raise SystemExit('Final splitting parity failed')
print('PASS',sum(g['values'] for g in groups.values()),'comparisons')
