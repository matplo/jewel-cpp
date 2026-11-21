"""Backward ISR timing against the unedited JEWEL timing block."""
import hashlib,json,math,subprocess,sys
from pathlib import Path
cpp,reference,output=sys.argv[1:]
rows=[];cases=[]
for scatter in [0,1]:
 for factor in [.25,1,2.5]:
  for depths in [(0,0),(1,1),(2,3),(4,1)]:
   for energy in [1,100,1000,-1,-100,-1000]:
    for emitted_energy in [1,-1]:
     for u in [1e-12,.25,.9,1-1e-12]:
      records={};index=107
      for side,depth in enumerate(depths):
       sign=1 if side==0 else -1;beam=3+side;previous=105+side
       records[previous]=[beam,energy*.1,-energy*.2,sign*energy*1.1,energy,-.75,0]
       for i in range(depth):
        incoming,emitted=index,index+1;index+=2
        records[previous][0]=incoming
        records[incoming]=[beam,.1*energy,.05*energy,sign*energy*1.2,energy,-.25*(i+1),0]
        nominal=.13957 if i%2 else 0.
        # Include exact equality and the adjacent representable mass above it.
        mass=nominal if i%3==0 else math.nextafter(nominal,math.inf) if nominal else .5
        records[emitted]=[incoming,.1,.2,sign*.3,emitted_energy,mass,nominal]
        previous=incoming
      n=index-1;draws=2+sum(max(0,d-1) for d in depths)+sum(records[i][-2]!=records[i][-1] for i in records if i>=108 and i%2==0)
      payload=f'{n} {factor} 1.2 -0.7 20 {scatter} 100\n'
      payload+=''.join(' '.join(format(v,'.17g') for v in records[i])+'\n' for i in range(105,n+1))
      payload+=' '.join(format(u,'.17g') for _ in range(100))+'\n'
      rows.append(payload);cases.append(dict(scatter=scatter,factor=factor,depths=depths,energy=energy,emitted_energy=emitted_energy,u=u,n=n,expected_draws=draws))
payload=''.join(rows);results=[]
for exe in [cpp,reference]:
 run=subprocess.run([exe],input=payload,text=True,capture_output=True,timeout=90)
 if run.returncode:raise RuntimeError((exe,run.stderr))
 results.append([float(v) for v in run.stdout.split()])
a,b=results;assert len(a)==len(b)
failures=[];maximum=0;cursor=0
for case in cases:
 assert a[cursor]==b[cursor]==case['expected_draws'],(case,a[cursor],b[cursor])
 size=1+5*(case['n']-104)
 for i in range(cursor+1,cursor+size):
  maximum=max(maximum,abs(a[i]-b[i]))
  if not math.isfinite(a[i]) or not math.isfinite(b[i]) or not math.isclose(a[i],b[i],rel_tol=2e-12,abs_tol=2e-13):
   failures.append(dict(case=case,index=i-cursor,cpp=a[i],reference=b[i]))
 cursor+=size
assert cursor==len(a)
report=dict(scope='backward ISR timing block only, supplied ancestry and nominal masses',cases=len(cases),
 signed_energy_cases=sum(c['energy']<0 or c['emitted_energy']<0 for c in cases),
 values=len(a),max_absolute_error=maximum,failures=failures,relative_tolerance=2e-12,absolute_tolerance=2e-13,
 input_sha256=hashlib.sha256(payload.encode()).hexdigest(),executables={str(p):hashlib.sha256(Path(p).read_bytes()).hexdigest() for p in [cpp,reference]},
 reference_sources=json.loads((Path(__file__).parents[1]/'reference/medium-source.json').read_text()))
Path(output).write_text(json.dumps(report,indent=2)+'\n')
print(len(cases),len(a),maximum,len(failures))
if failures:raise SystemExit(str(failures[:3]))
