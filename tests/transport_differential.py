import hashlib,json,math,random,subprocess,sys
from pathlib import Path
cpp,reference,output=sys.argv[1:]
failures=[];cases=0;accepted=0;max_error=0.;max_draws=0;records=[]
for boost,floor in [(0,.45),(1,.45),(0,2.)]:
 base=f'CONFIG\n208 3 1 .4 .59 .17 .17 .54 6.199999809265137 3 2 {floor} 1\nXCONFIG\n3 1 .2 .2 50 1 {boost}\n'
 commands=[]
 for pdg in [1,21]:
  for elastic in [0,1]:
   for birth in [(0,0,0,0),(3,1,0,.4),(15,0,0,.4),(0,0,.3,.4)]:
    for start,dt,lifetime,scale in [(0,10,20,1),(.5,2,20,1),(1,.01,20,1),(1,5,2,1),(5,2,2,1),(1,0,20,1),(1,5,20,0),(1,20,30,2)]:
     rng=random.Random(2600+len(commands));tape=[rng.uniform(.000001,.999999) for _ in range(3000)]
     # Include nonzero longitudinal velocity while keeping timelike sampled positions.
     pz=2 if birth[2] else 0
     values=[10,0,pz,math.sqrt(100+pz*pz+.09),.3,pdg,*birth,start,dt,lifetime,scale,elastic]
     commands.append('TIME\n'+' '.join(format(x,'.17g') for x in values)+'\n3000\n'+' '.join(format(x,'.17g') for x in tape)+'\n')
 payload=base+''.join(commands)
 rows=[]
 for exe in [cpp,reference]:
  result=subprocess.run([exe],input=payload,text=True,capture_output=True)
  if result.returncode:raise RuntimeError((exe,result.returncode,result.stderr[-2000:]))
  rows.append([list(map(float,line.split())) for line in result.stdout.splitlines()])
 assert len(rows[0])==len(rows[1])==len(commands)
 for i,(a,b) in enumerate(zip(*rows)):
  assert len(a)==len(b)==3
  cases+=1;accepted+=int(b[0]);max_error=max(max_error,abs(a[1]-b[1]));max_draws=max(max_draws,int(b[2]))
  if a[0]!=b[0] or a[2]!=b[2] or not math.isclose(a[1],b[1],rel_tol=2e-12,abs_tol=2e-13):failures.append(dict(config=[boost,floor],case=i,cpp=a,fortran=b))
 records.append(dict(boost=boost,debye_floor=floor,cases=len(commands),input_sha256=hashlib.sha256(payload.encode()).hexdigest()))
report=dict(scope='GETDELTAT collision-time replay, not full transport',cases=cases,accepted=accepted,max_absolute_time_error=max_error,max_draws=max_draws,failures=failures,records=records,relative_tolerance=2e-12,absolute_tolerance=2e-13,sources=json.loads((Path(__file__).parents[1]/'reference/medium-source.json').read_text()),executables={Path(exe).name:hashlib.sha256(Path(exe).read_bytes()).hexdigest() for exe in [cpp,reference]})
Path(output).write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({k:v for k,v in report.items() if k not in ['sources','executables','records']},indent=2))
if failures:raise SystemExit('Transport parity failed')
