"""Replay complete captured legacy MPI draw tapes with native cross sections."""
import argparse,gzip,hashlib,json,math,subprocess
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('cpp',type=Path);p.add_argument('directory',type=Path);p.add_argument('output',type=Path);p.add_argument('--width-config',type=Path,required=True);p.add_argument('--alpha-s',type=float,required=True);p.add_argument('--native-output',type=Path);p.add_argument('--write-fixture',type=Path);a=p.parse_args()
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
if a.directory.is_file():
 with gzip.open(a.directory,'rt') as f:fixture=json.load(f)
 assert fixture['schema']==1
 sequences=fixture['sequences'];provenance=fixture['provenance'];evidence='archived-reference-fixture'
else:
 sequences=[];current=None;state=None
 for line in (a.directory/'run.log').read_text().splitlines():
  t=line.split()
  if not t or not t[0].startswith('SEQ_'):continue
  tag=t[0];values=list(map(float,t[1:]))
  if tag=='SEQ_BEGIN':
   assert current is None and values[1:]==[0,0,0,8,0,13100,2,1],values
   current=dict(config=None,xconfig=None,draws=[],states=[],sigmas=[])
  elif tag=='SEQ_CONFIG':assert len(values)==13;current['config']=values
  elif tag=='SEQ_XCONFIG':assert len(values)==23;current['xconfig']=values
  elif tag=='SEQ_DRAW':current['draws'].append(values[0])
  elif tag=='SEQ_SIGMA':current['sigmas'].append(values[0])
  elif tag=='SEQ_STATE':
   assert len(values)==12;state=dict(checkpoint=len(current['draws']),values=values);current['states'].append(state)
  elif tag in ['SEQ_PARTICLE','SEQ_STRING']:state['values'].extend(values)
  elif tag=='SEQ_END':
   for s in current['states']:assert len(s['values'])==12+5*int(s['values'][9])+2*int(s['values'][10])
   sequences.append(current);current=None;state=None
 assert current is None and len(sequences)==10
 provenance=dict(hepmc_byte_identical=sha(a.directory/'events.hepmc')==sha(a.directory/'baseline/events.hepmc'),
  baseline_hepmc_sha256=sha(a.directory/'baseline/events.hepmc'),
  artifacts={name:sha(a.directory/name) for name in ['pythia.f','trace.f90','reference','params.dat','medium.dat','run.log','instrumentation.patch']})
 evidence='live-reference-trace'
if a.write_fixture:
 a.write_fixture.parent.mkdir(parents=True,exist_ok=True)
 a.write_fixture.write_bytes(gzip.compress(json.dumps(dict(schema=1,sequences=sequences,provenance=provenance),separators=(',',':')).encode(),mtime=0))
payload=''
for seq in sequences:
 for row in [seq['config'],seq['xconfig'],seq['states'][0]['values'],[len(seq['draws']),*seq['draws']]]:
  payload+=' '.join(format(v,'.17g') for v in row)+'\n'
run=subprocess.run([str(a.cpp.resolve()),str(a.width_config.resolve()),format(a.alpha_s,'.17g')],input=payload,capture_output=True,text=True,timeout=120)
if a.native_output:a.native_output.write_text(run.stdout)
assert run.returncode==0,(run.returncode,run.stderr[-2000:])
native=[];current=dict(states=[],sigmas=[])
for line in run.stdout.splitlines():
 t=line.split()
 if t[0]=='STATE':current['states'].append(dict(checkpoint=int(t[1]),values=list(map(float,t[2:]))))
 elif t[0]=='SIGMA':current['sigmas'].append(float(t[1]))
 elif t[0]=='END':native.append(current);current=dict(states=[],sigmas=[])
 else:raise AssertionError(line)
assert len(native)==len(sequences)
failures=[];groups={};step_count=0;accepted=0
for event,(ref,cpp) in enumerate(zip(sequences,native)):
 assert len(ref['states'])==len(cpp['states']) and len(ref['sigmas'])==len(cpp['sigmas']),(event,len(ref['states']),len(cpp['states']),len(ref['sigmas']),len(cpp['sigmas']))
 step_count+=len(ref['states'])-1;accepted+=int(ref['states'][-1]['values'][6]-ref['states'][0]['values'][6])
 def compare(name,x,y,exact=False):
  g=groups.setdefault(name,dict(values=0,maximum_absolute_error=0.,maximum_relative_error=0.));g['values']+=1;g['maximum_absolute_error']=max(g['maximum_absolute_error'],abs(x-y));g['maximum_relative_error']=max(g['maximum_relative_error'],abs(x-y)/max(abs(y),1e-300))
  if not math.isfinite(x) or not math.isfinite(y) or (x!=y if exact else not math.isclose(x,y,rel_tol=2e-12,abs_tol=2e-13)):
   if len(failures)<30:failures.append(dict(sequence=event,group=name,cpp=x,fortran=y))
 for left,right in zip(cpp['states'],ref['states']):
  compare('draw-checkpoints',left['checkpoint'],right['checkpoint'],True)
  v,w=left['values'],right['values'];assert len(v)==len(w)
  n=int(w[9]);integer={5,6,9,10,11}|set(range(12,12+5*n,5))|set(range(12+5*n,len(w)))
  for j,(x,y) in enumerate(zip(v,w)):compare('state-integers' if j in integer else 'state-scalars',x,y,j in integer)
 for x,y in zip(cpp['sigmas'],ref['sigmas']):compare('cross-sections',x,y)
identity=provenance['hepmc_byte_identical']
report=dict(scope='full native screened MPI component sequences with Pythia8 coupling and LHAPDF; captured initial state/draws; MPI showers disabled',sequences=len(sequences),steps=step_count,accepted_pairs=accepted,random_draws=sum(len(s['draws']) for s in sequences),groups=groups,failures=failures,hepmc_byte_identical=identity,baseline_hepmc_sha256=provenance['baseline_hepmc_sha256'],reference_evidence=evidence,cpp_sha256=sha(a.cpp),input_sha256=hashlib.sha256(payload.encode()).hexdigest(),width_config_sha256=sha(a.width_config),alpha_s_mz=a.alpha_s,relative_tolerance=2e-12,absolute_tolerance=2e-13,artifacts=provenance['artifacts'])
a.output.write_text(json.dumps(report,indent=2)+'\n');assert identity and not failures,failures[:5]
print('PASS',step_count,'native steps;',accepted,'pairs;',report['random_draws'],'draws; reference evidence:',evidence)
