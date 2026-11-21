"""Compare surviving emitted-line times, including singular direct-beam inputs."""
import hashlib,json,math,subprocess,sys
from pathlib import Path
cpp,reference,output=sys.argv[1:]
cases=[];payloads=[]
for scatter in [0,1]:
 for depths in [(0,0),(0,1),(2,0),(1,3)]:
  for energy in [-10,10]:
   for u in [1e-12,.25,.9,1-1e-12]:
    records={};emitted_ids=[];index=107
    for side,depth in enumerate(depths):
     previous=105+side;beam=3+side;sign=1 if side==0 else -1
     records[previous]=[beam,1,2,sign*12,energy,-1 if depth else 0,0]
     for i in range(depth):
      parent,emitted=index,index+1;index+=2
      records[previous][0]=parent
      records[parent]=[beam,1,2,sign*15,energy,-.5,0]
      records[emitted]=[parent,.1,.2,sign*.3,2,.5 if i%2 else 0,0]
      emitted_ids.append(emitted);previous=parent
    n=index-1
    draws=2+sum(max(0,d-1)+d//2 for d in depths)
    payload=f'{n} 1 1.2 -.7 20 {scatter} 100\n'
    payload+=''.join(' '.join(map(str,records[i]))+'\n' for i in range(105,n+1))
    payload+=' '.join(format(u,'.17g') for _ in range(100))+'\n'
    payloads.append(payload);cases.append(dict(n=n,depths=depths,scatter=scatter,energy=energy,u=u,draws=draws,emitted=emitted_ids))
payload=''.join(payloads);results=[]
for exe,args in [(cpp,['--emissions-only']),(reference,[])]:
 run=subprocess.run([exe,*args],input=payload,text=True,capture_output=True,check=True,timeout=90)
 results.append([float(v) for v in run.stdout.split()])
a,b=results
assert len(a)==len(b)
cursor=0;compared=0;nonfinite_discarded=0;maximum=0
for case in cases:
 assert a[cursor]==b[cursor]==case['draws'],case
 for line in range(105,case['n']+1):
  offset=cursor+1+5*(line-105)
  for x,y in zip(a[offset:offset+5],b[offset:offset+5]):
   if line in case['emitted']:
    assert math.isfinite(x) and math.isfinite(y) and math.isclose(x,y,rel_tol=2e-12,abs_tol=2e-13),(case,line,x,y)
    compared+=1;maximum=max(maximum,abs(x-y))
   else:
    assert x==0,'Emission projection retained an incoming record'
    nonfinite_discarded+=not math.isfinite(y)
 cursor+=1+5*(case['n']-104)
assert cursor==len(a) and nonfinite_discarded>0
report=dict(scope='Surviving ISR emission times and random consumption; no incoming timing output',
 cases=len(cases),emitted_values=compared,random_count_comparisons=len(cases),
 discarded_reference_nonfinite_values=nonfinite_discarded,max_absolute_error=maximum,
 relative_tolerance=2e-12,absolute_tolerance=2e-13,input_sha256=hashlib.sha256(payload.encode()).hexdigest(),
 executables={Path(exe).name:hashlib.sha256(Path(exe).read_bytes()).hexdigest() for exe in [cpp,reference]},
 reference_block_sha256=hashlib.sha256((Path(reference).parent/'isr-times-reference.f').read_bytes()).hexdigest(),
 test_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),failures=[])
Path(output).write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
