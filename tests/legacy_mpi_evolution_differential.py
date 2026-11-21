"""Stateful screened MPI sequences with injected, identical cross sections."""
import collections,hashlib,json,math,random,subprocess,sys
from pathlib import Path
cpp,reference,output=sys.argv[1:];rng=random.Random(2600421);events=[]
for index in range(90):
 steps=80;screen=8.588417995819353e-7;bound=1000.
 remaining=[(.9,.8),(.02,.02),(.6,.4)][index%3]
 config=[5020,.01,screen,.830599328667382,2,bound,45.73389,*remaining,.9,.9,.3,2*math.pi,1,steps,*[(.5,.3,.4),(1,0,0),(0,1,0)][index%3]]
 if index%10==0:config[1]=.005*screen
 if index%10==1:config[4]=0
 if index%10==2:config[13]=239
 if index%4==0:config[9:11]=[0,0]
 elif index%4==1:config[9:11]=[1,1]
 rows=[]
 for step in range(steps):
  ratio=[0,.2,1,2][index%4] if index<45 else [0,.2,1,2][step%4]
  rows.append([ratio*bound,*[rng.random() for _ in range(10)]])
 events.append((config,rows))
def encode(events):
 return ''.join(' '.join(format(v,'.17g') for v in config)+'\n'+''.join(' '.join(format(v,'.17g') for v in r)+'\n' for r in rows) for config,rows in events)
payload=encode(events)
def run(exe):
 r=subprocess.run([exe],input=payload,capture_output=True,text=True,timeout=60);assert r.returncode==0,(exe,r.stderr[-2000:])
 rows=[list(map(float,line.split())) for line in r.stdout.splitlines()];assert len(rows)==sum(len(rows) for _,rows in events)
 return rows
a,b=run(cpp),run(reference);failures=[];comparisons=0;maximum=0.;outcomes=collections.Counter();offset=0;accepted=0
for event,(config,inputs) in enumerate(events):
 previous=None
 for step in range(len(inputs)):
  left,right=a[offset],b[offset];offset+=1
  if len(left)!=len(right):raise AssertionError((event,step,len(left),len(right)))
  n,nstr=int(left[12]),int(left[13]);assert len(left)==15+5*n+2*nstr
  exact={0,1,2,8,9,12,13,14}|set(range(15,15+5*n,5))|set(range(15+5*n,len(left)))
  for j,(x,y) in enumerate(zip(left,right)):
   comparisons+=1;maximum=max(maximum,abs(x-y))
   if not math.isfinite(x) or not math.isfinite(y) or (x!=y if j in exact else not math.isclose(x,y,rel_tol=2e-12,abs_tol=2e-13)):
    if len(failures)<30:failures.append(dict(event=event,step=step,column=j,cpp=x,fortran=y))
  status=int(left[0]);outcomes[status]+=1
  if status==4:
   accepted+=1;assert left[1] in [9,10] and left[2]==1
   assert left[4]>=.01 and left[5]>=.01
  elif status==2:assert left[1]==6 and left[2]==0
  elif status==3:assert left[1]==7 and left[2]==1
  else:assert status==1 and left[1] in [0,1] and left[2]==0 and left[14]==1
  if previous is not None:
   assert left[3]<=previous[3]
   if status!=4:assert left[4:14]==previous[4:14] and left[15:]==previous[15:]
   if previous[14]:assert status==1 and left[1]==0
  assert math.isclose(left[4]+left[6],config[7],rel_tol=0,abs_tol=2e-14)
  assert math.isclose(left[5]+left[7],config[8],rel_tol=0,abs_tol=2e-14)
  previous=left
assert set(outcomes)=={1,2,3,4}
report=dict(scope='composed PYMULT state transitions with injected cross sections, initial strings and no MPI showers',events=len(events),steps=len(a),scalar_comparisons=comparisons,maximum_absolute_error=maximum,outcomes=dict(outcomes),accepted_pairs=accepted,failures=failures,relative_tolerance=2e-12,absolute_tolerance=2e-13,input_sha256=hashlib.sha256(payload.encode()).hexdigest(),source_sha256='9fcfed664e4ac34462aa63679f2dd3e935c9f6f63f7cd6a785b73ccb82740ed9',executables={Path(e).name:hashlib.sha256(Path(e).read_bytes()).hexdigest() for e in [cpp,reference]})
Path(output).write_text(json.dumps(report,indent=2)+'\n');assert not failures,failures[:5]
print('PASS',comparisons,'state values;',dict(outcomes),'accepted pairs',accepted)
