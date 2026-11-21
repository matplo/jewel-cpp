#!/usr/bin/env python3
"""Numerical parity and exact RNG consumption, not event-level physics validation."""
import hashlib, json, math, random, subprocess, sys
from pathlib import Path
cpp, reference, report_path = sys.argv[1:]
cases=[]
def add(op,nf,a,b=0,c=0,tape=None):
    text=f"{op} {nf} {a:.17g} {b:.17g} {c:.17g}\n"
    if tape is not None:
        text+=str(len(tape))+"\n"
        if tape: text+=" ".join(format(r,'.17g') for r in tape)+"\n"
    cases.append((op,text))
for nf in (0,3,4,5,6):
    for z in (1e-12,1e-8,1e-4,.01,.1,.25,.5,.75,.9,.99,1-1e-8,1-1e-12):
        for op in ('PQQ','PGQ','PGG','PQG'): add(op,nf,z)
        for q in (1.,10.,100.,1000.):
            for op in ('INTPQQ','INTPGGLOW','INTPGGHIGH'):
                factor=z if op=='INTPGGLOW' else 1-z
                if math.log(q*q/.04)+math.log(factor)>0: add(op,nf,z,q)
    for lam in (.1,.2,.3):
        for factor in (1+1e-8,1.01,2.,10.,1e3,1e8): add('ALPHAS',nf,lam*lam*factor,lam)
    # Explicit rejection followed by acceptance; empty interval; degenerate interval.
    add('GETT',nf,.1,100,.5,[.99,.99,.01,1e-8])
    add('GETT',nf,2.,1.,.5,[])
    add('GETT',nf,1.,1.,.5,[.2,.9])
    r=random.Random(2600+nf)
    for i in range(80):
        add('GETT',nf,10**r.uniform(-2,1),10**r.uniform(1.1,4),r.uniform(.3,2),
            [r.uniform(1e-12,1-1e-12) for _ in range(1000)])
payload=''.join(text for _,text in cases)
def run(exe):
    result=subprocess.run([exe],input=payload,text=True,capture_output=True,check=True)
    rows=[(float(line.split()[0]),int(line.split()[1])) for line in result.stdout.splitlines()]
    if len(rows)!=len(cases): raise AssertionError(f'{exe}: output count mismatch')
    return rows
actual, expected=run(cpp),run(reference)
summary={}
failures=[]
for index,((op,_),(a,ad),(b,bd)) in enumerate(zip(cases,actual,expected)):
    error=abs(a-b); relative=error/max(abs(b),1e-300)
    row=summary.setdefault(op,dict(cases=0,max_absolute_error=0.,max_relative_error=0.,max_draws=0))
    row['cases']+=1
    row['max_absolute_error']=max(row['max_absolute_error'],error)
    row['max_relative_error']=max(row['max_relative_error'],relative)
    row['max_draws']=max(row['max_draws'],ad)
    if not math.isfinite(a) or not math.isclose(a,b,rel_tol=2e-12,abs_tol=2e-13) or ad!=bd:
        failures.append(dict(index=index,kernel=op,cpp=a,fortran=b,cpp_draws=ad,fortran_draws=bd))
report=dict(scope='nine extracted stock kernels; not full events',cases=len(cases),
    relative_tolerance=2e-12,absolute_tolerance=2e-13,
    input_sha256=hashlib.sha256(payload.encode()).hexdigest(),
    source=json.loads((Path(__file__).parents[1]/'reference/source.json').read_text()),
    executables={Path(x).name:hashlib.sha256(Path(x).read_bytes()).hexdigest() for x in (cpp,reference)},
    kernels=summary,failures=failures)
Path(report_path).write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(summary,indent=2))
if failures: raise SystemExit(f'{len(failures)} differential failures; see {report_path}')
print(f'PASS: {len(cases)} comparisons, including exact random draw counts')
