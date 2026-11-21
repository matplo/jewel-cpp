"""Probe negative recoil sentinel versus zero at the full collision boundary.

Each reference case is bounded separately: an invalid source candidate must
never hold the entire regression batch open. Nonfinite and exhausted runs
are counted explicitly, not treated as numerical parity.
"""
from concurrent.futures import ThreadPoolExecutor
import hashlib
import itertools
import json
import math
from pathlib import Path
import random
import subprocess
import sys

cpp, reference, output = sys.argv[1:]
base = 'CONFIG\n208 3 1 .4 .59 .17 .17 .54 6.199999809265137 3 2 .45 1\nBCONFIG\n3 1 .2 1 0\n'
def row(values):
    return ' '.join(format(v,'.17g') for v in values)+'\n'

def case(item):
    index, (mode,status,target,fraction,energy_fraction) = item
    mass, energy = 2.,20.
    norm = math.sqrt(energy*energy-mass*mass)
    momentum = [.3*norm,.4*norm,math.sqrt(.75)*norm,energy]
    rng = random.Random(29000+index)
    payload = base+'SEQUENCE\n'+row([mode,1,1,target,3,energy_fraction,fraction,1,0,1,1])
    payload += row([*momentum,mass,21,status,0,0,0,.4,.1,.2,.3,.4,7,-1,-1,10,11,20])
    payload += row([1,0,0,1,1,0,1,0,0,3])
    payload += row([-.2,1.21,21,.2,-.3,.4,math.sqrt(1+.04+.09+.16),1,.1,.2,.3,1.2])
    payload += '5000\n'+row([rng.uniform(.000001,.999999) for _ in range(5000)])
    result = dict(case=index,mode=mode,status=status,target=target,fraction=fraction,energy_fraction=energy_fraction,
                  fixture_sha256=hashlib.sha256(payload.encode()).hexdigest())
    records = []
    for label, binary in [('cpp',cpp),('fortran',reference)]:
        try:
            run = subprocess.run([binary],input=payload,text=True,capture_output=True,timeout=3)
        except subprocess.TimeoutExpired:
            result[label] = dict(timeout_seconds=3)
            records.append(None)
            continue
        lines = [list(map(float,line.split())) for line in run.stdout.splitlines()]
        result[label] = dict(returncode=run.returncode,header=lines[0] if lines else None)
        records.append(lines)
    a,b = records
    if not a or not b or result['cpp'].get('returncode') or result['fortran'].get('returncode'):
        result['classification'] = 'incomplete execution'
    elif b[0][-1]:
        result['classification'] = 'shared rejection' if a[0][-1] else 'unexpected native acceptance'
    elif a[0][-1]:
        result['classification'] = 'unexpected native rejection'
    else:
        # Finite source outcomes must agree in active K/P/V/MV record data.
        n = int(b[0][0]); mismatch = []
        if a[0] != b[0]:mismatch.append('header')
        if len(a) != len(b):mismatch.append('output length')
        for i,(ar,br) in enumerate(zip(a[1:n+1],b[1:n+1]),start=1):
            for column,(x,y) in enumerate(zip(ar,br)):
                if column in (3,4) and y>n:y=0 # cleared inactive daughter links
                if not (math.isfinite(x) and math.isfinite(y) and math.isclose(x,y,rel_tol=2e-12,abs_tol=2e-13)):
                    mismatch.append([i,column,x,y])
        result['classification'] = 'finite parity' if not mismatch else 'finite mismatch'
        result['mismatches'] = mismatch[:10]
    return result

settings = list(itertools.product(range(4),[2,14],[0,2,4,20],[-1,0],[1,.6]))
with ThreadPoolExecutor(max_workers=2) as pool:
    results = list(pool.map(case,enumerate(settings)))
counts = {label:sum(r['classification']==label for r in results) for label in sorted({r['classification'] for r in results})}
report = dict(scope='negative recoil fraction and zero control; full DOKINEMATICS; no full-event parity claim',
              cases=len(results),counts=counts,results=results,
              executables={p:hashlib.sha256(Path(p).read_bytes()).hexdigest() for p in [cpp,reference]},
              source=json.loads((Path(__file__).parents[1]/'reference/medium-source.json').read_text()))
Path(output).write_text(json.dumps(report,indent=2,allow_nan=False)+'\n')
print(counts,flush=True)
if any(r['classification'] not in ('finite parity','shared rejection') for r in results):
    raise SystemExit('Unresolved fraction comparison; inspect report')
assert counts.get('finite parity',0)>0 and counts.get('shared rejection',0)>0
