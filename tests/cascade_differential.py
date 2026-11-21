"""Whole MAKEBRANCH and MAKECASCADE replay against frozen JEWEL."""
import argparse,hashlib,json,math,random,subprocess,sys
from pathlib import Path
parser=argparse.ArgumentParser()
for name in ['cpp','reference','output']:parser.add_argument(name)
parser.add_argument('--baseline',action='store_true')
parser.add_argument('--generated',action='store_true',help='Use splitmix64-open52-v1 instead of a finite Python-random tape')
parser.add_argument('--guarded',action='store_true',help='Use bounded diagnostic reference scheduling, not stock MAKECASCADE')
parser.add_argument('--case',type=int,default=None,dest='case_index')
parser.add_argument('--native-input',action='store_true',help='Replay variable-length input with a nonzero generated RNG cursor')
parser.add_argument('--compression-padding',type=int,default=None)
parser.add_argument('--seed-offset',type=int,default=0)
parser.add_argument('--coherence',choices=['fixture','off','on'],default='fixture')
args=parser.parse_args();cpp,reference,output=args.cpp,args.reference,args.output;baseline_only=args.baseline
sys.path.insert(0,str(Path(__file__).parents[1]/'tools'))
from cascade_cases import build_cases
base,cases,labels,case_indices=build_cases(baseline_only=baseline_only,guarded=args.guarded,generated=args.generated,case_index=args.case_index,seed_offset=args.seed_offset,coherence=None if args.coherence=="fixture" else args.coherence=="on")
if args.native_input:
 if not args.generated or args.compression_padding is not None:parser.error('native input needs generated RNG and no padding')
 selected=[i for i,label in enumerate(labels) if label['operation'] in ['CASCADE','GCASCADE']]
 transformed=[]
 for i in selected:
  lines=cases[i].splitlines();index=lines.index(labels[i]['operation']);lines[index]='NCASCADE'
  lines.insert(index+3,'5');lines.append('17')
  transformed.append('\n'.join(lines)+'\n');labels[i]['operation']='NCASCADE';labels[i]['rng_cursor']=17
 cases=transformed;labels=[labels[i] for i in selected];case_indices=[case_indices[i] for i in selected]
 args.guarded=True
if args.compression_padding is not None:
 if not 0<=args.compression_padding<=22995:parser.error('invalid compression padding')
 selected=[i for i,label in enumerate(labels) if label['operation'] in ['CASCADE','GCASCADE']]
 transformed=[]
 for i in selected:
  lines=cases[i].splitlines();index=lines.index(labels[i]['operation']);lines[index]='CCASCADE'
  lines.insert(index+8,str(args.compression_padding))
  transformed.append('\n'.join(lines)+'\n');labels[i]['operation']='CCASCADE';labels[i]['padding']=args.compression_padding
 cases=transformed;labels=[labels[i] for i in selected];case_indices=[case_indices[i] for i in selected]
capacity=64
payload=base+''.join(cases)
def run(exe):
 try:
  r=subprocess.run([exe],input=payload,text=True,capture_output=True,timeout=240)
 except subprocess.TimeoutExpired as error:
  for suffix,data in [('out',error.stdout),('err',error.stderr)]:
   if isinstance(data,bytes):data=data.decode(errors='replace')
   Path(output).with_suffix('.'+Path(exe).name+'.'+suffix).write_text(data or '')
  raise
 Path(output).with_suffix('.'+Path(exe).name+'.out').write_text(r.stdout)
 Path(output).with_suffix('.'+Path(exe).name+'.err').write_text(r.stderr)
 if r.returncode:raise RuntimeError((exe,r.returncode,r.stderr[-3000:]))
 rows=iter(r.stdout.splitlines());out=[]
 for _ in cases:
  h=list(map(float,next(rows).split()));n=int(h[0]);out.append([h]+[list(map(float,next(rows).split())) for _ in range(n+103+capacity)])
 assert next(rows,None) is None
 return out
a,b=run(cpp),run(reference);failures=[];values=0;maximum=0.
for index,(actual,expected) in enumerate(zip(a,b)):
 if actual[0]!=expected[0] or expected[0][6]:failures.append(dict(case=case_indices[index],header_cpp=actual[0],header_fortran=expected[0]));continue
 n=int(expected[0][0]);assert len(actual)==len(expected)
 for i,(ar,br) in enumerate(zip(actual,expected)):
  assert len(ar)==len(br)
  for j,(x,y) in enumerate(zip(ar,br)):
   values+=1;maximum=max(maximum,abs(x-y))
   discrete=i==0 or (1<=i<=n and (j<5 or 21<=j<=24 or j==26)) or (n+2<=i<=n+101 and j==6) or (n+102<=i<=n+101+capacity and j==0)
   if not math.isfinite(x) or not (x==y if discrete else math.isclose(x,y,rel_tol=2e-12,abs_tol=2e-13)):
    failures.append(dict(case=case_indices[index],row=i,column=j,cpp=x,fortran=y))
report=dict(scope="MAKEBRANCH and MAKECASCADE",baseline_only=baseline_only,guarded=args.guarded,rng="splitmix64-open52-v1" if args.generated else "python-random-tape",case_indices=case_indices,cases=len(cases),values=values,max_absolute_error=maximum,failures=failures,settings=labels,
 max_draws=max(int(r[0][4]) for r in b),max_records=max(int(r[0][0]) for r in b),input_sha256=hashlib.sha256(payload.encode()).hexdigest(),
 sources=json.loads((Path(__file__).parents[1]/'reference/medium-source.json').read_text()),
 guard_source_sha256=hashlib.sha256((Path(__file__).parents[1]/'reference/guarded_cascade.f90').read_bytes()).hexdigest() if args.guarded else None,
 executables={Path(exe).name:hashlib.sha256(Path(exe).read_bytes()).hexdigest() for exe in [cpp,reference]})
Path(output).write_text(json.dumps(report,indent=2)+'\n')
print({k:v for k,v in report.items() if k not in ['sources','executables','settings','failures']})
if failures:print(failures[:30]);raise SystemExit(f'{len(failures)} scheduler mismatches')
