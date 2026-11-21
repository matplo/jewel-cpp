"""Analyze isolated Pythia/source traces; requires byte-identical diagnostic output."""
import json,hashlib,math,subprocess,sys
from pathlib import Path
root=Path(sys.argv[1])
source={};native={}
for line in (root/'both1500.err').read_text().splitlines():
 v=line.split()
 if not v:continue
 if v[0]=='ISR_SOURCE':
  a=list(map(float,v[1:]));source[tuple(a[1:7])]=a
 if v[0]=='ISR_NATIVE':
  a=list(map(float,v[1:]));native[tuple(a[:6])]=a
rows=[json.loads(l) for l in (root/'both1500.jsonl').open()]
report=dict(scope='Diagnostic comparison of source and reconstructed ISR recoil matrices; no tolerance change',
 identical_source_instrumentation=(root/'baseline1500.jsonl').read_bytes()==(root/'events1500.jsonl').read_bytes(),
 identical_double_instrumentation=(root/'baseline1500.jsonl').read_bytes()==(root/'both1500.jsonl').read_bytes(),failures=[])
assert report['identical_source_instrumentation'] and report['identical_double_instrumentation']
for e in rows:
 if e['type']!='event' or e['subsystem_isr_frame_error'] is None:continue
 failure=dict(attempt=e['attempt'],error=e['subsystem_isr_frame_error'],branches=[])
 for f in e['isr_beam_fractions']:
  key=(f['mother'],f['emitted'],*f['before'],*f['after'])
  if key not in native:continue
  s,n=source[key],native[key];sm,nm=s[14:],n[10:]
  assert len(sm)==len(nm)==16
  copies=[]
  for after_id in range(max(f['mother'],f['recoiler'])+1,f['emitted']):
   after=e['particles'][after_id-1];before=e['particles'][after['mothers'][0]-1]
   v=[before['energy_before_beam_repair'],*before['momentum'][:3]]
   actual=[after['energy_before_beam_repair'],*after['momentum'][:3]]
   def apply(m):
    line=' '.join(format(x,'.17g') for x in m+v)+'\n'
    return list(map(float,subprocess.run([str(root/'apply')],input=line,capture_output=True,text=True,check=True).stdout.split()))
   sp,np=apply(sm),apply(nm)
   copies.append(dict(before=before['id'],after=after_id,source_max_error=max(abs(x-y) for x,y in zip(sp,actual)),
    native_max_error=max(abs(x-y) for x,y in zip(np,actual)),
    source_outside_tolerance=any(abs(x-y)>2e-9+2e-11*abs(y) for x,y in zip(sp,actual)),
    native_outside_tolerance=any(abs(x-y)>2e-9+2e-11*abs(y) for x,y in zip(np,actual))))
  if any(c['native_outside_tolerance'] for c in copies):
   failure['branches'].append(dict(emitted=f['emitted'],source=dict(m2=s[7],pt2=s[8],z=s[9],q2=s[10],phi=s[13],matrix=sm),
    reconstructed=dict(m2=n[6],z=n[7],q2=n[8],phi=n[9],matrix=nm),
    max_matrix_entry=max(map(abs,sm)),max_matrix_difference=max(abs(x-y) for x,y in zip(sm,nm)),copies=copies))
 report['failures'].append(failure)
report['files_sha256']={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in [root/'SimpleSpaceShower.cc',root/'native_frames.cpp',root/'libpythia8.dylib',root/'both',root/'both1500.err',root/'both1500.jsonl',root/'baseline1500.jsonl',root/'apply.cpp',root/'apply']}
(root/'report.json').write_text(json.dumps(report,indent=2)+'\n')
for f in report['failures']:
 print(f['attempt'])
 for b in f['branches']:print('matrix_max',b['max_matrix_entry'],'delta',b['max_matrix_difference'],'phi_delta',b['reconstructed']['phi']-b['source']['phi'],'copies',b['copies'])
