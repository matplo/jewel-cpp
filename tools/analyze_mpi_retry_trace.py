"""Compare adaptive native screening initialization with complete reference retries."""
import argparse,gzip,hashlib,json,math,subprocess
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('cpp',type=Path);p.add_argument('reference',type=Path);p.add_argument('output',type=Path);p.add_argument('--width-config',type=Path,required=True);p.add_argument('--alpha-s',type=float,required=True);p.add_argument('--write-fixture',type=Path);p.add_argument('--lifecycle',action='store_true');a=p.parse_args()
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
if a.reference.is_file():
 with gzip.open(a.reference,'rt') as f:data=json.load(f)
 assert data['schema']==1;records=data['records'];provenance=data['provenance'];evidence='archived-reference-fixture'
else:
 records=[];passes=[];current=None
 for line in (a.reference/'run.log').read_text().splitlines():
  t=line.split()
  if not t or not t[0].startswith('INIT_'):continue
  v=list(map(float,t[1:]));tag=t[0]
  if tag=='INIT_BEGIN':
   if current is not None:assert current['integral']<1.1*current['config'][4]
   assert v[1:-1]==[4,8,0,13100,2,1] and v[-1] in ([1,3] if a.lifecycle else [1]),v
   current=dict(samples=int(v[0]),stage=int(v[-1]),config=None,draws=[],points=[],bins=[]);passes.append(current)
  elif tag=='INIT_CONFIG':assert len(v)==31;current['config']=v
  elif tag=='INIT_DRAW':current['draws'].append(v[0])
  elif tag=='INIT_SAMPLE':current['points'].append(v)
  elif tag=='INIT_BIN':current['bins'].append(v)
  elif tag=='INIT_INTEGRAL':current['integral']=v[0]
  elif tag=='INIT_OVERLAP':current['overlap']=v
  elif tag=='INIT_END':records.append(passes);passes=[];current=None
 assert current is None and not passes and len(records)==(8 if a.lifecycle else 4)
 for channel in records:
  for r in channel:
   assert len(r['points'])==20*r['samples'] and len(r['draws'])==120*r['samples'] and len(r['bins'])==20
   assert all(r['config'][j]==channel[0]['config'][j] for j in range(31) if j not in [1,5,28])
  assert channel[-1]['integral']>=1.1*channel[-1]['config'][4]
 provenance=dict(hepmc_byte_identical=sha(a.reference/'events.hepmc')==sha(a.reference/'baseline/events.hepmc'),baseline_hepmc_sha256=sha(a.reference/'baseline/events.hepmc'),artifacts={name:sha(a.reference/name) for name in ['pythia.f','core.f','trace.f90','reference','params.dat','medium.dat','run.log','instrumentation.patch','configuration.patch']})
 evidence='live-reference-trace'
if a.lifecycle and evidence=='live-reference-trace':
 assert [r[0]['stage'] for r in records]==[1,3]*4
 records=[first+second for first,second in zip(records[::2],records[1::2])]
payload=''
for channel in records:
 r=channel[0];draws=[u for r in channel for u in r['draws']]
 for row in [[r['samples'],*r['config']],[len(draws),*draws]]:payload+=' '.join(format(v,'.17g') for v in row)+'\n'
run=subprocess.run([str(a.cpp.resolve()),str(a.width_config.resolve()),format(a.alpha_s,'.17g')]+(['--lifecycle'] if a.lifecycle else []),input=payload,capture_output=True,text=True,timeout=180);assert run.returncode==0,run.stderr[-2000:]
native=[];passes=[];current=None;saved=None
for line in run.stdout.splitlines():
 t=line.split();v=list(map(float,t[1:]));tag=t[0]
 if tag=='PASS':assert int(v[0])==len(passes);current=dict(parameters=v[1:],points=[],bins=[]);passes.append(current)
 elif tag=='SAMPLE':current['points'].append(v)
 elif tag=='BIN':passes[int(v[0])]['bins'].append(v[1:])
 elif tag=='INTEGRAL':passes[int(v[0])]['integral']=v[1]
 elif tag=='OVERLAP':passes[-1]['overlap']=v
 elif tag=='SAVED':saved=v
 elif tag=='END':native.append(dict(passes=passes,draws=int(v[0]),saved=saved));passes=[];current=None;saved=None
 else:raise AssertionError(line)
assert len(native)==len(records);groups={};failures=[]
for channel,(cpp,ref) in enumerate(zip(native,records)):
 def compare(name,x,y,exact=False):
  g=groups.setdefault(name,dict(values=0,max_absolute_error=0.,max_relative_error=0.));g['values']+=1;g['max_absolute_error']=max(g['max_absolute_error'],abs(x-y));g['max_relative_error']=max(g['max_relative_error'],abs(x-y)/max(abs(y),1e-300))
  if not math.isfinite(x) or not math.isfinite(y) or (x!=y if exact else not math.isclose(x,y,rel_tol=2e-12,abs_tol=2e-13)):
   if len(failures)<30:failures.append(dict(channel=channel,group=name,cpp=x,fortran=y))
 compare('pass-count',len(cpp['passes']),len(ref),True);assert len(cpp['passes'])==len(ref)
 compare('draw-count',cpp['draws'],sum(len(r['draws']) for r in ref),True)
 for index,(c,r) in enumerate(zip(cpp['passes'],ref)):
  for u,v in zip(c['parameters'],[r['config'][28],r['config'][1],r['config'][5]]):compare('screening-parameters',u,v)
  assert len(c['points'])==len(r['points']) and len(c['bins'])==20
  for x,y in zip(c['points'],r['points']):
   for j,(u,v) in enumerate(zip(x,y)):compare('cross-sections' if j==4 else 'sampled-kinematics',u,v)
  for x,y in zip(c['bins'],r['bins']):
   for j,(u,v) in enumerate(zip(x,y)):compare('bin-counts' if j<2 else 'bin-sums',u,v,j<2)
  compare('integral',c['integral'],r['integral'])
  compare('acceptance',int(c['integral']>=1.1*r['config'][4]),int('overlap' in r),True)
 if a.lifecycle:
  legacy=next(r for r in ref if r['stage']==1 and 'overlap' in r)
  for u,v in zip(cpp['saved'],legacy['overlap']):compare('saved-legacy-overlap',u,v)
 for u,v in zip(cpp['passes'][-1]['overlap'],ref[-1]['overlap']):compare('overlap-normalization',u,v)
report=dict(scope=('paired PYMULT/PYMIGN stages with native calculated tables' if a.lifecycle else 'adaptive PYMULT screening with native calculated tables/normalization; controlled initial pT0Ref=4 GeV'),channels=[dict(beams=r[0]['config'][10:12],passes=len(r),final_pt0_reference=c['passes'][-1]['parameters'][0],final_integral=c['passes'][-1]['integral']) for r,c in zip(records,native)],sampled_points=sum(len(p['points']) for r in records for p in r),random_draws=sum(len(p['draws']) for r in records for p in r),groups=groups,failures=failures,reference_evidence=evidence,provenance=provenance,input_sha256=hashlib.sha256(payload.encode()).hexdigest(),cpp_sha256=sha(a.cpp),width_config_sha256=sha(a.width_config),relative_tolerance=2e-12,absolute_tolerance=2e-13)
a.output.write_text(json.dumps(report,indent=2)+'\n');assert provenance['hepmc_byte_identical'] and not failures,failures[:5]
if a.write_fixture:
 a.write_fixture.parent.mkdir(parents=True,exist_ok=True);a.write_fixture.write_bytes(gzip.compress(json.dumps(dict(schema=1,records=records,provenance=provenance),separators=(',',':')).encode(),mtime=0))
print('PASS',report['sampled_points'],'points;',report['random_draws'],'draws;',report['channels'])
