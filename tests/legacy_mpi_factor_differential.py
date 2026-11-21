"""PYSIGH screened MPI common factor before matrix elements and PDFs."""
import hashlib,json,math,subprocess,sys
from pathlib import Path
cpp,reference,output=sys.argv[1:]
cases=[]
for c in [[1,0,.5,.5,0],[0,1,0,0,1],[.3,.7,.2,.3,.5],[.5,.5,1,0,0]]:
 for s in [.1,200**2,5020**2]:
  for xt in [1e-10,1e-6,.1,.9,1-1e-8]:
   for tau in [xt,math.sqrt(xt),(1+xt)/2]:
    half=-.5*math.log(tau)
    for y in [-.9999*half,-.5*half,0,.5*half,.9999*half]:
     for screen in [8.5884179958193529e-7,1.060298518002389e-6,.01]:
      for k in [0,1,2]:cases.append([s,xt,tau,y,screen,.389379,k,*c])
def encode(rows):return ''.join(' '.join(format(x,'.17g') for x in row)+'\n' for row in rows)
payload=encode(cases)
def run(exe):
 result=subprocess.run([exe],input=payload,capture_output=True,text=True,timeout=60)
 assert result.returncode==0,(exe,result.stderr,len(result.stdout.splitlines()))
 return list(map(float,result.stdout.split()))
a,b=run(cpp),run(reference);assert len(a)==len(b)==len(cases)
failures=[];absolute=relative=0.
for i,(x,y) in enumerate(zip(a,b)):
 absolute=max(absolute,abs(x-y));relative=max(relative,abs(x-y)/max(abs(y),1e-300))
 if not math.isfinite(x) or not math.isfinite(y) or not math.isclose(x,y,rel_tol=2e-12,abs_tol=2e-13) or ((x==0)!=(y==0)):
  failures.append(dict(case=i,cpp=x,fortran=y,input=cases[i]))
base=[5020**2,.01,.1,0,1e-6,.389379,1,.3,.7,.2,.3,.5];invalid=[]
for col,value in [(0,0),(1,0),(1,1),(2,.001),(2,1),(3,10),(4,0),(5,0),(6,-1),(7,-1)]:
 row=list(base);row[col]=value;invalid.append(row)
row=list(base);row[7:9]=[0,0];invalid.append(row)
row=list(base);row[9:]=[0,0,0];invalid.append(row)
for row in invalid:
 result=subprocess.run([cpp],input=encode([row]),capture_output=True,text=True)
 assert result.returncode!=0,row
report=dict(scope='PYSIGH ISTSB=9 screened common factor only; matrix elements and PDFs excluded',cases=len(cases),maximum_absolute_error=absolute,maximum_relative_error=relative,relative_tolerance=2e-12,absolute_tolerance=2e-13,failures=failures,invalid_cases=len(invalid),input_sha256=hashlib.sha256(payload.encode()).hexdigest(),source_sha256='9fcfed664e4ac34462aa63679f2dd3e935c9f6f63f7cd6a785b73ccb82740ed9',executables={Path(e).name:hashlib.sha256(Path(e).read_bytes()).hexdigest() for e in [cpp,reference]})
Path(output).write_text(json.dumps(report,indent=2)+'\n');assert not failures,failures[:10]
print('PASS',len(cases),'common-factor comparisons;',len(invalid),'invalid inputs; max relative error',relative)
