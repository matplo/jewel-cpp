"""Compare the original GENEVENT survivor selection and ordered preparation."""
import collections,hashlib,itertools,json,math,struct,subprocess,sys
from pathlib import Path
cpp,reference,output=sys.argv[1:]
factor=struct.unpack('f',struct.pack('f',1.00001))[0]
nominal={0:0,1:.33,2:.33,4:1.5,5:4.8,6:173.,21:0,22:0,211:.13957}
def row(pdg,status,mass,eta=0):
 p=[1.,0.,math.sinh(eta)];energy=math.sqrt(sum(x*x for x in p)+mass*mass)
 return [status,pdg,*p,energy,mass,.1,.2,.3,1.,nominal[abs(pdg)]]
def hard(pdg,sign):return [4,pdg,3.*sign,4.*sign,12.*sign,13.,0.,.4,.5,.6,2.,nominal[abs(pdg)]]
cases=[]
for pdg in [0,1,-1,4,-5,6,21,22,211]:
 threshold=nominal[abs(pdg)]*factor
 masses=[0,threshold,math.nextafter(threshold,math.inf) if threshold else .1]
 if threshold:masses+=[math.nextafter(threshold,0)]
 for status in [1,2,4,10,11,21]:
  for mass in masses:
   for eta in [0,math.nextafter(2.,0),2.,math.nextafter(2.,math.inf),3.]:
    cases.append(dict(kind='selection-boundaries',records=[hard(2,1),row(pdg,status,mass,eta),hard(21,-1)],first=1,second=3,limit=2.))
# Multiple low-virtuality conversions must update the hard pair in slot order.
for permutation in itertools.permutations([row(1,1,.33,.1),row(4,2,1.5,1.),row(-5,4,4.8,-1.)]):
 for placement in [0,1]:
  records=[hard(2,1),*permutation,hard(21,-1)] if placement==0 else [permutation[0],hard(2,1),permutation[1],hard(21,-1),permutation[2]]
  cases.append(dict(kind='ordered-conversions',records=records,first=1 if placement==0 else 2,second=5 if placement==0 else 4,limit=2.))
# Source hard-line status setup, including a non-QCD hard leg and history codes.
for pdg,status in itertools.product([2,6,22],[1,4,10,11,21]):
 records=[hard(pdg,1),row(1,1,.33),hard(21,-1)];records[0][0]=status
 cases.append(dict(kind='hard-status',records=records,first=1,second=3,limit=2.))
payload=''.join(f"PREPARE\n{len(c['records'])} {c['first']} {c['second']} {c['limit']}\n"+''.join(' '.join(format(x,'.17g') for x in row)+'\n' for row in c['records']) for c in cases)
results=[]
for exe in [cpp,reference]:
 run=subprocess.run([exe],input=payload,text=True,capture_output=True,check=True,timeout=120)
 results.append([float(v) for v in run.stdout.split()])
a,b=results;assert len(a)==len(b)==sum(len(c['records'])*10 for c in cases)
groups={};failures=[];cursor=0;status_counts=collections.Counter()
for case_id,case in enumerate(cases):
 group=groups.setdefault(case['kind'],dict(cases=0,values=0,max_absolute_error=0.));group['cases']+=1
 for row_id in range(len(case['records'])):
  for col in range(10):
   x,y=a[cursor],b[cursor];cursor+=1;group['values']+=1;group['max_absolute_error']=max(group['max_absolute_error'],abs(x-y))
   okay=x==y if col==0 else math.isfinite(x) and math.isclose(x,y,rel_tol=2e-12,abs_tol=2e-13)
   if not okay:failures.append(dict(case=case_id,row=row_id,col=col,cpp=x,reference=y))
   if col==0:status_counts[int(x)]+=1
report=dict(scope='GENEVENT survivor status selection and ordered MAKEMASSLESS; not full event mapping',
 cases=len(cases),groups=groups,status_counts=dict(status_counts),failures=failures,
 relative_tolerance=2e-12,absolute_tolerance=2e-13,input_sha256=hashlib.sha256(payload.encode()).hexdigest(),
 sources=json.loads((Path(__file__).parents[1]/'reference/kinematics-source.json').read_text()),
 executables={Path(exe).name:hashlib.sha256(Path(exe).read_bytes()).hexdigest() for exe in [cpp,reference]})
Path(output).write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k] for k in ['cases','groups','status_counts']},indent=2))
if failures:print(failures[:10]);raise SystemExit(f'{len(failures)} preparation mismatches')
