"""Replay projected native events through the frozen, safe-range Fortran probe.

Nominal masses are supplied identically: this tests the projection operation,
not Pythia6/Pythia8 mass-table equivalence or the whole generator.
"""
import argparse,hashlib,json,math,subprocess
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('input',type=Path);p.add_argument('reference',type=Path);p.add_argument('output',type=Path)
a=p.parse_args();records=[json.loads(line) for line in a.input.read_text().splitlines()]
header=records[0];mode=header.get('subtraction',{}).get('put_on_shell',0)
if mode not in [1,2]:raise SystemExit('Requires projected events in mode 1 or 2')
events=[e for e in records if e['type']=='event'];payload=[]
if not events:raise SystemExit('No accepted events to validate')
for e in events:
 before,after=e['subtracted_particles'],e['projected_particles']
 assert len(before)==len(after) and before,'Reference requires nonempty selected range'
 payload.append(f'{len(before)} {mode} 0\n')
 for old,new in zip(before,after):payload.append(' '.join(format(v,'.17g') for v in [*old['momentum'],old['mass'],new['nominal_mass']])+'\n')
payload=''.join(payload);run=subprocess.run([str(a.reference.resolve())],input=payload,text=True,capture_output=True,check=True,timeout=120)
lines=run.stdout.splitlines();offset=0;results=[];failures=[]
for e in events:
 after=e['projected_particles'];n=int(lines[offset]);offset+=1;assert n==len(after)
 maximum=0;values=0
 for row,particle in enumerate(after):
  expected=list(map(float,lines[offset].split()));offset+=1;assert len(expected)==5
  for col,(x,y) in enumerate(zip([*particle['momentum'],particle['mass']],expected)):
   values+=1;maximum=max(maximum,abs(x-y))
   if not(math.isfinite(x) and math.isfinite(y) and math.isclose(x,y,rel_tol=2e-12,abs_tol=2e-13)):
    failures.append(dict(attempt=e['attempt'],particle=row,column=col,cpp=x,reference=y))
 results.append(dict(attempt=e['attempt'],particles=n,values=values,max_absolute_error=maximum,projection=e['projection']))
assert offset==len(lines)
report=dict(scope=__doc__.strip(),mode=mode,attempts=header['attempts_requested'],accepted=len(events),failed_attempts=[{k:e[k] for k in ['attempt','type','stage','reason'] if k in e} for e in records if e['type'].endswith('failure')],results=results,failures=failures,relative_tolerance=2e-12,absolute_tolerance=2e-13,input_sha256=hashlib.sha256(a.input.read_bytes()).hexdigest(),fixture_sha256=hashlib.sha256(payload.encode()).hexdigest(),reference_sha256=hashlib.sha256(a.reference.read_bytes()).hexdigest(),source=json.loads((Path(__file__).parents[1]/'docs/source-manifest.json').read_text())['jewel-2.6.0.f'])
a.output.write_text(json.dumps(report,indent=2,allow_nan=False)+'\n')
print(json.dumps(dict(mode=mode,accepted=len(events),values=sum(r['values'] for r in results),max_absolute_error=max(r['max_absolute_error'] for r in results),mismatches=len(failures))))
if failures:raise SystemExit('Projection comparison failed')
