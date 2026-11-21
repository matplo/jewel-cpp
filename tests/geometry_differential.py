"""Compare native straight-line estimates with custom JEWEL's unchanged JG_LENGTHS."""
import hashlib,json,math,subprocess,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
manifest=json.loads((root/'reference/geometry-source.json').read_text())
body=(root/'reference/geometry_lengths.f').read_text().split('      SUBROUTINE JG_FAIL')[0]
assert hashlib.sha256(body.encode()).hexdigest()==manifest['routine_sha256'], 'geometry reference body changed'
cpp,reference,output=map(Path,sys.argv[1:]);count=0;maximum=0.;failures=[];inputs=[]
for b in [0,3,7]:
 for woods in [0,1]:
  base=f'CONFIG\n208 3 {woods} .4 .59 .17 .17 .54 6.199999809265137 {b} 2 .45 1\n'
  table=subprocess.run([str(reference)],input=base+'TABLE\n',capture_output=True,text=True,check=True)
  radius=float(table.stdout.splitlines()[-1].split()[0])
  queries=[]
  for x,y in [(0,0),(2,-1),(radius-b/2,0)]:
   for momentum in [(10,0,0,10),(-6,0,8,10),(0,6,-8,10),(0,0,8,10),(0,0,0,2)]:
    for step in [.02,.1,.007]:
     for lifetime in [.2,1,30]:
      queries.append('LENGTHS\n'+' '.join(format(v,'.17g') for v in [x,y,b,radius,lifetime,*momentum,step])+'\n')
  # Vacuum backend convention exits before momentum validation/integration.
  queries.append('LENGTHS\n0 0 0 0 30 0 0 0 0 .02\n')
  payload=base+''.join(queries);values=[]
  for exe in [cpp,reference]:
   result=subprocess.run([str(exe)],input=payload,capture_output=True,text=True,check=True,timeout=60)
   values.append([float(v) for v in result.stdout.split()])
  assert len(values[0])==len(values[1])==4*len(queries)
  for i,(a,z) in enumerate(zip(*values)):
   count+=1;maximum=max(maximum,abs(a-z))
   if not math.isfinite(a) or not math.isfinite(z) or (a==0)!=(z==0) or not math.isclose(a,z,abs_tol=2e-13,rel_tol=2e-12):
    failures.append(dict(b=b,woods=woods,query=i//4,component=i%4,cpp=a,reference=z))
  inputs.append(dict(b=b,woods=woods,queries=len(queries),input_sha256=hashlib.sha256(payload.encode()).hexdigest()))
report=dict(values=count,max_absolute_error=maximum,failures=failures,inputs=inputs,
 absolute_tolerance=2e-13,relative_tolerance=2e-12,zero_nonzero_agreement=True,
 medium_sources=json.loads((root/'reference/medium-source.json').read_text()),
 sources=json.loads((Path(__file__).resolve().parents[1]/'reference/geometry-source.json').read_text()),
 executables={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in [cpp,reference]})
output.write_text(json.dumps(report,indent=2)+'\n');print(count,maximum,len(failures))
if failures:raise SystemExit(str(failures[:5]))
