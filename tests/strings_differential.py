"""Full MAKESTRINGS_VAC/CLEANUP replay; compare literal K/P/V/colour slots."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import random
import subprocess

parser=argparse.ArgumentParser(description=__doc__)
for key in ['cpp','reference','output']:parser.add_argument(key,type=Path)
parser.add_argument('--sample',type=Path)
args=parser.parse_args()
rng=random.Random(2602600)
cases=[];labels=[]
def record(status,pdg,col=0,anti=0,index=0):
 return [status,pdg,0,0,0,index+.1,index+.2,index+.3,index+3.,.2,1.,2.,3.,4.,5.,6.,7.,8.,9.,10.,col,anti]
def add(rows,label):
 for compress in [0,1]:cases.append((rows,compress));labels.append(dict(case=label,compress=compress))
for i in range(90):
 length=i%12;status=[1,3,4,5][i%4];q=[1,2,5,-1,-3,2101,-2203][i%7]
 # Colour orientation, not PDG sign, determines traversal in this routine.
 chain=[record(status,q,101,0,1)]
 for j in range(length):chain.append(record(status,21,102+j,101+j,j+2))
 chain.append(record(status,-q,0,101+length,length+2))
 rng.shuffle(chain)
 if i%3==0:chain.insert(0,record(4,22,0,0,40))
 if i%4==0:chain.insert(0,record(11,21,777,778,41))
 add([record(0,0)]+chain,f'chain-{i}')
 if i<30:
  broken=[r[:] for r in chain];next(r for r in broken if r[21]==101)[21]=999
  add([record(0,0)]+broken,f'broken-{i}')
add([record(0,0),record(4,21,101,102),record(4,21,102,101)],'closed-gluon-loop')
add([record(0,0),record(4,21,101,999)],'loose-gluon')
add([record(0,0),record(4,21,101,999),record(17,21,999,101)],'inactive-partner')
add([record(0,0),record(4,22),record(4,211),record(4,11)],'colourless-active')
add([record(0,0),record(11,21),record(12,21),record(13,21),record(14,21)],'inactive-only')
add([record(0,0),record(4,1,101),record(4,-1,0,101),record(4,2,102),record(4,-2,0,102)],'two-strings')
# Capacity failure increments N before rejecting; compare the partial record.
for n in [22988,22990]:
 rows=[record(0,0)]+[record(17,0) for _ in range(n-3)]+[record(4,1,101),record(4,-1,0,101)]
 add(rows,f'capacity-{n}')
if args.sample:
 for event in map(json.loads,args.sample.read_text().splitlines()):
  if event['type']!='event':continue
  rows=[]
  for p in event['particles']:
   rows.append([p['status'],p['pdg'],0,0,0,*p['momentum'],p['mass'],*[0.]*5,*p['production_fm'],p['decay_time_fm'],p['colour'],p['anticolour']])
  add(rows,f"native-{event['attempt']}")
payload=''.join(f'{len(rows)} {compress}\n'+''.join(' '.join(format(v,'.17g') for v in r)+'\n' for r in rows) for rows,compress in cases)
def run(exe):
 result=subprocess.run([str(exe.resolve())],input=payload,text=True,capture_output=True,timeout=90)
 args.output.with_suffix('.'+exe.name+'.err').write_text(result.stderr)
 if result.returncode:raise RuntimeError((exe,result.returncode,result.stderr[-1000:]))
 lines=iter(result.stdout.splitlines());events=[]
 for _ in cases:
  h=list(map(int,next(lines).split()));events.append((h,[list(map(float,next(lines).split())) for _ in range(h[0])]))
 assert next(lines,None) is None
 return events
cpp,ref=run(args.cpp),run(args.reference);failures=[];values=0
for index,((a,ar),(b,br)) in enumerate(zip(cpp,ref)):
 if a!=b:failures.append(dict(case=index,cpp=a,reference=b));continue
 for row,(x,y) in enumerate(zip(ar,br)):
  values+=len(x)
  if x!=y:failures.append(dict(case=index,row=row,cpp=x,reference=y))
report=dict(scope='full MAKESTRINGS_VAC + CLEANUP slot-level replay',cases=len(cases),values=values,failures=failures,
 labels=labels,discarded=sum(h[1] for h,_ in ref),empty_successes=sum(h==[0,0] for h,_ in ref),
 input_sha256=hashlib.sha256(payload.encode()).hexdigest(),
 source=json.loads((Path(__file__).parents[1]/'reference/medium-source.json').read_text())['jewel-2.6.0.f.stock']['sha256'],
 executables={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in [args.cpp,args.reference]})
args.output.write_text(json.dumps(report,indent=2)+'\n')
print({k:v for k,v in report.items() if k not in ['labels','failures','executables']})
if failures:print(failures[:3]);raise SystemExit(f'{len(failures)} string mismatches')
