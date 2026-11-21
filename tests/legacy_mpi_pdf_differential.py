"""Native LHAPDF versus full frozen PYPDFU and source heavy-flavour suppression."""
import hashlib,json,math,struct,subprocess,sys
from pathlib import Path
cpp,reference,output=sys.argv[1:]
factor=struct.unpack('f',struct.pack('f',1.1))[0]
scales=[0,.1,1.,1.69,10.,100.,1e6]
for mass in [1.5,4.8]:
 edge=factor*(mass*mass)
 scales.extend([math.nextafter(edge,0),edge,math.nextafter(edge,math.inf)])
cases=[]
for beam in [2212,2112,-2212,-2112]:
 for x in [0,1e-10,1e-5,.1,.8,1,1.1]:
  for remaining in [1,.5,.1]:
   for q2 in scales:
    for nf in [0,3,5,6]:cases.append([beam,x,remaining,q2,nf,1.5,4.8])
def encode(rows):return ''.join(' '.join(format(x,'.17g') for x in row)+'\n' for row in rows)
payload=encode(cases)
def run(command):
 r=subprocess.run(command,input=payload,capture_output=True,text=True,timeout=120)
 assert r.returncode==0,(command,r.stderr[-2000:])
 rows=[]
 for line in r.stdout.splitlines():
  fields=line.split()
  if len(fields)!=11:continue
  try:row=list(map(float,fields))
  except ValueError:continue
  rows.append(row)
 assert len(rows)==len(cases),(command,len(rows),len(cases),r.stdout[:1000])
 return rows,r.stderr
failures=[];maximum=relative=0.;policies=[]
for policy in ['linked-common','metadata']:
 b,stderr=run([reference]+(['metadata'] if policy=='metadata' else []))
 floor=float(next(line.split()[1] for line in stderr.splitlines() if line.strip().startswith('PDF_FLOOR')))
 a,_=run([cpp,'CT14nlo','0']+([format(floor,'.17g')] if policy=='linked-common' else []))
 policies.append(dict(policy=policy,reference_q2_floor=floor,scalar_comparisons=11*len(cases)))
 for i,(left,right) in enumerate(zip(a,b)):
  for j,(x,y) in enumerate(zip(left,right)):
   maximum=max(maximum,abs(x-y));relative=max(relative,abs(x-y)/max(abs(y),1e-300))
   if not math.isfinite(x) or not math.isfinite(y) or not math.isclose(x,y,rel_tol=2e-12,abs_tol=2e-13) or ((x==0)!=(y==0)):
    failures.append(dict(policy=policy,case=i,column=j,cpp=x,fortran=y,input=cases[i]))
invalid=[];base=[2212,.1,1,10,5,1.5,4.8]
for col,value in [(0,22),(2,0),(2,1.1),(3,-1),(4,7),(5,-1),(6,-1)]:
 row=list(base);row[col]=value;invalid.append(row)
for row in invalid:
 r=subprocess.run([cpp,'CT14nlo','0'],input=encode([row]),capture_output=True,text=True);assert r.returncode!=0,row
for floor in ['-1','nan','inf']:
 r=subprocess.run([cpp,'CT14nlo','0',floor],input=encode([base]),capture_output=True,text=True)
 assert r.returncode!=0,floor
report=dict(scope='legacy MSTP52=2 MSTP57=1 nucleon PDFs and PYSIGH heavy suppression; no companion/nuclear model',set='CT14nlo/0',reference_pdf_id=13100,cases=len(cases),scalar_comparisons=22*len(cases),policies=policies,maximum_absolute_error=maximum,maximum_relative_error=relative,failures=failures,relative_tolerance=2e-12,absolute_tolerance=2e-13,zero_decisions_exact=True,invalid_cases=len(invalid),invalid_floor_cases=3,input_sha256=hashlib.sha256(payload.encode()).hexdigest(),source_sha256='9fcfed664e4ac34462aa63679f2dd3e935c9f6f63f7cd6a785b73ccb82740ed9',executables={Path(e).name:hashlib.sha256(Path(e).read_bytes()).hexdigest() for e in [cpp,reference]})
Path(output).write_text(json.dumps(report,indent=2)+'\n');assert not failures,failures[:10]
print('PASS',report['scalar_comparisons'],'full-PYPDFU comparisons; max absolute',maximum)
