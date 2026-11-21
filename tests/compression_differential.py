"""Exact COMPRESSEVENT comparison, including inactive physical slots."""
import hashlib,json,random,subprocess,sys
from pathlib import Path
cpp,reference,output=sys.argv[1:]
rng=random.Random(26008832)
cases=[]
# Empty removal, protected disposable records, exact >10 boundary, first slot.
for count in [1,2,10,11,12,13,32]:
 for status in [1,11,12,13,14,15,20]:
  for selected in sorted({0,count//2,count}):cases.append(([status]*count,selected))
for _ in range(80):
 n=rng.randrange(2,160)
 cases.append(([rng.choice([1,2,3,4,5,11,12,13,14,19,20]) for _ in range(n)],rng.randrange(n+1)))
cases.extend([([11 if i%3 else 1 for i in range(23000)],22999),([1]*23000,23000)])
parts=[]
for statuses,selected in cases:
 parts.append(f'{len(statuses)} {selected}\n')
 for slot,status in enumerate(statuses):
  row=[status,slot+100,slot+1,slot+2,slot+3]
  row += [(slot*15+j-100)*.125 for j in range(15)]
  row += [slot+10,slot+20,slot*.25,slot*.5,slot%2,(slot//2)%2,slot*1.25-1e6,(slot//3)%2]
  parts.append(' '.join(map(str,row))+'\n')
payload=''.join(parts)
def run(exe):
 result=subprocess.run([exe],input=payload,text=True,capture_output=True,check=True,timeout=120)
 return iter(result.stdout.splitlines())
a,b=run(cpp),run(reference)
values=0;failures=[];removed=0
for case,(statuses,selected) in enumerate(cases):
 for row in range(len(statuses)+1):
  x=list(map(float,next(a).split()));y=list(map(float,next(b).split()))
  if row==0:removed+=len(statuses)-int(y[0])
  values+=len(y)
  if x!=y:failures.append({'case':case,'row':row,'cpp':x,'reference':y})
assert next(a,None) is None and next(b,None) is None
report={'scope':'COMPRESSEVENT boundary records, not scheduler integration','cases':len(cases),
 'values':values,'removed_total':removed,'failures':failures,
 'input_sha256':hashlib.sha256(payload.encode()).hexdigest(),
 'source_sha256':json.loads((Path(__file__).parents[1]/'reference/medium-source.json').read_text())['jewel-2.6.0.f.stock']['sha256'],
 'executables':{Path(e).name:hashlib.sha256(Path(e).read_bytes()).hexdigest() for e in [cpp,reference]}}
Path(output).write_text(json.dumps(report,indent=2)+'\n')
print({k:v for k,v in report.items() if k not in ['executables','failures']})
if failures:print(failures[:3]);raise SystemExit(f'{len(failures)} compression mismatches')
