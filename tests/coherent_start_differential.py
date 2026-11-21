"""Compare unchanged DOCOHSCATTERING initialization, including all record writes."""
import hashlib,json,math,subprocess,sys
from pathlib import Path
cpp,reference,output=sys.argv[1:]
cases=[]
def record(status,pdg,parent,daughters,momentum,mass,colour,anti):
 return [status,pdg,parent,*daughters,*momentum,mass,1,-2,3,.4,2,-1,3,.4,9,.3,.2,0,colour,anti]
for pdg,first,second,direction in [(21,21,21,1),(21,21,21,-1),(21,1,-1,0),(1,1,21,0),(-1,-1,21,0)]:
 for selected in [3,4]:
  for momentum in [(0,0,20,math.sqrt(416)),(3,4,20,21),(-3,4,-20,21)]:
   for mode in ['normal','shorten','zero_time','negative_interval','disabled','mother_status','sibling_status','expired','negative_angle','root_mother','selected_status']:
    root=record(11,21,0,[0,0],momentum,4,101,102)
    mother=record(11,pdg,1,[3,4],momentum,4,101,102)
    a=record(1,first,2,[0,0],[1,2,8,9],.3,101 if direction>=0 else 103,104)
    b=record(1,second,2,[0,0],[2,2,12,12],.5,104,102)
    a[14:18]=[1,2,3,.3];b[14:18]=[-1,2,3,.5]
    sibling=b if selected==3 else a
    enabled,start,duration,angle=1,.4,2,.3
    if mode=='shorten':sibling[18]=.8
    if mode=='zero_time':duration=0
    if mode=='negative_interval':duration=-1
    if mode=='disabled':enabled=0
    if mode=='mother_status':mother[0]=12
    if mode=='sibling_status':sibling[0]=2
    if mode=='expired':sibling[18]=.1
    if mode=='negative_angle':angle=-1
    if mode=='root_mother':mother[2]=0
    if mode=='selected_status':(a if selected==3 else b)[0]=2
    cases.append((mode,[selected,enabled,start,duration,angle],[root,mother,a,b]))
payload=''.join(' '.join(format(x,'.17g') for x in header)+'\n'+''.join(' '.join(format(x,'.17g') for x in row)+'\n' for row in records) for _,header,records in cases)
def run(exe):
 r=subprocess.run([exe],input=payload,text=True,capture_output=True)
 if r.returncode:raise RuntimeError((exe,r.returncode,r.stderr[-2000:]))
 rows=iter(r.stdout.splitlines());out=[]
 for _ in cases:
  header=list(map(float,next(rows).split()));n=int(header[-1]);out.append((header,[list(map(float,next(rows).split())) for _ in range(n)]))
 assert next(rows,None) is None
 return out
actual,expected=run(cpp),run(reference);failures=[];groups={};maximum=0;values=0
for i,((mode,_,_),a,b) in enumerate(zip(cases,actual,expected)):
 g=groups.setdefault(mode,dict(cases=0,merged=0));g['cases']+=1;g['merged']+=int(a[0][0])
 if len(a[0])!=len(b[0]) or len(a[1])!=len(b[1]):failures.append(dict(case=i,reason='merge/count mismatch'));continue
 for row,(x,y) in enumerate(zip([a[0]]+a[1],[b[0]]+b[1])):
  assert len(x)==len(y)
  for col,(v,w) in enumerate(zip(x,y)):
   values+=1;maximum=max(maximum,abs(v-w))
   discrete=(row==0 and (col==0 or col>=5 or col==len(x)-1)) or (row>0 and (col<5 or col>=21))
   if (discrete and v!=w) or not math.isfinite(v) or not math.isclose(v,w,rel_tol=2e-12,abs_tol=2e-13):failures.append(dict(case=i,row=row,column=col,cpp=v,fortran=w))
report=dict(scope='DOCOHSCATTERING eligibility and initial merge only',cases=len(cases),groups=groups,values=values,max_absolute_error=maximum,failures=failures,
 relative_tolerance=2e-12,absolute_tolerance=2e-13,input_sha256=hashlib.sha256(payload.encode()).hexdigest(),
 sources=json.loads((Path(__file__).parents[1]/'reference/coherence-source.json').read_text()),
 executables={Path(exe).name:hashlib.sha256(Path(exe).read_bytes()).hexdigest() for exe in [cpp,reference]})
Path(output).write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(groups,indent=2));print(values,maximum)
if failures:print(failures[:10]);raise SystemExit('coherent start mismatch')
