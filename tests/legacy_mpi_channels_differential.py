"""ISUB=96 channel-by-channel parity, including threshold decisions and PDF order."""
import hashlib,json,math,subprocess,sys
from pathlib import Path
cpp,reference,output=sys.argv[1:]
cases=[]
for masses in [[.33,.33,.5,1.5,4.8],[0,0,0,1.3,4.2]]:
 energies={1.,10.,100.,10000.}
 for mass in masses[3:]:
  edge=4.04*mass*mass
  energies.update([math.nextafter(edge,0),edge,math.nextafter(edge,math.inf),edge*(1-1e-10),edge*(1+1e-10)])
 for sh in sorted(energies):
  for angle in [-.99,-.2,0,.6,.99]:
   for interference in [0,1]:
    for enabled in [[1]*5,[0]*5,[1,0,1,1,0]]:
     i=len(cases);factor=[1e-5,1,100][i%3];faca=[.5,1,2][i%3]
     pa=[(k+1)*.013 for k in range(11)];pb=[(11-k)*.021 for k in range(11)]
     if i%5==0:pa[1]=0;pb[4]=0
     if i%7==0:pa[2]=-.001 # diagnostic signed input, not a physical PDF model.
     cases.append([sh,-.5*sh*(1-angle),-.5*sh*(1+angle),angle,factor,.118,faca,4.2,interference,*masses,*enabled,*pa,*pb])
def encode(rows):return ''.join(' '.join(format(x,'.17g') for x in row)+'\n' for row in rows)
payload=encode(cases)
def run(exe):
 result=subprocess.run([exe],input=payload,capture_output=True,text=True,timeout=120)
 assert result.returncode==0,(exe,result.stderr)
 lines=iter(result.stdout.splitlines());records=[]
 for case in cases:
  header=next(lines).split();n=int(header[0]);total=float(header[1]);channels=[]
  for _ in range(n):
   row=next(lines).split();assert len(row)==5
   channels.append([*map(int,row[:3]),*map(float,row[3:])])
  records.append((total,channels))
 assert next(lines,None) is None
 return records
a,b=run(cpp),run(reference);maximum=relative=0.;failures=[];values=0;channels=0
for i,((total_a,left),(total_b,right)) in enumerate(zip(a,b)):
 assert len(left)==len(right),(i,len(left),len(right))
 pairs=[('total',total_a,total_b)]
 for j,(x,y) in enumerate(zip(left,right)):
  assert x[:3]==y[:3],(i,j,x,y)
  channels+=1
  pairs.extend([(f'{j}:matrix',x[3],y[3]),(f'{j}:pdf',x[4],y[4])])
 for label,x,y in pairs:
  values+=1;maximum=max(maximum,abs(x-y));relative=max(relative,abs(x-y)/max(abs(y),1e-300))
  if not math.isfinite(x) or not math.isfinite(y) or not math.isclose(x,y,rel_tol=2e-12,abs_tol=2e-13) or ((x==0)!=(y==0)):
   failures.append(dict(case=i,label=label,cpp=x,fortran=y))
invalid=[];base=cases[0]
for col,value in [(0,0),(1,0),(2,1),(3,1.1),(4,0),(5,0),(6,-1),(7,-1),(8,2),(9,-1),(20,float('nan'))]:
 row=list(base);row[col]=value;invalid.append(row)
for row in invalid:
 result=subprocess.run([cpp],input=encode([row]),capture_output=True,text=True)
 assert result.returncode!=0,row
report=dict(scope='PYSGQC ISUB=96 channel factors and supplied PDF multiplication; PYWIDT/PDF evaluation excluded',cases=len(cases),channel_comparisons=channels,numeric_comparisons=values,maximum_absolute_error=maximum,maximum_relative_error=relative,failures=failures,relative_tolerance=2e-12,absolute_tolerance=2e-13,channel_counts_and_order_exact=True,invalid_cases=len(invalid),input_sha256=hashlib.sha256(payload.encode()).hexdigest(),source_sha256='9fcfed664e4ac34462aa63679f2dd3e935c9f6f63f7cd6a785b73ccb82740ed9',executables={Path(e).name:hashlib.sha256(Path(e).read_bytes()).hexdigest() for e in [cpp,reference]})
Path(output).write_text(json.dumps(report,indent=2)+'\n');assert not failures,failures[:10]
print('PASS',channels,'channel records and',values,'numeric comparisons; max relative',relative)
