"""Native MPI stratified initialization and overlap against recorded PYMULT."""
import argparse,gzip,hashlib,json,math,subprocess
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('cpp',type=Path);p.add_argument('reference',type=Path);p.add_argument('output',type=Path);p.add_argument('--baseline',type=Path);p.add_argument('--width-config',type=Path,required=True);p.add_argument('--alpha-s',type=float,required=True);p.add_argument('--write-fixture',type=Path);a=p.parse_args()
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
if a.reference.is_file():
 with gzip.open(a.reference,'rt') as f:data=json.load(f)
 assert data['schema']==1;records=data['records'];provenance=data['provenance'];evidence='archived-reference-fixture'
else:
 records=[];current=None
 for line in (a.reference/'run.log').read_text().splitlines():
  t=line.split()
  if not t or not t[0].startswith('INIT_'):continue
  values=list(map(float,t[1:]));tag=t[0]
  if tag=='INIT_BEGIN':
   assert current is None, 'Retry pass requires explicit grouping before replay'
   assert values[1:]==[4,8,0,13100,2,1,1],values
   current=dict(samples=int(values[0]),config=None,draws=[],points=[],bins=[],overlap=None)
  elif tag=='INIT_CONFIG':assert len(values)==28;current['config']=values
  elif tag=='INIT_DRAW':current['draws'].append(values[0])
  elif tag=='INIT_SAMPLE':current['points'].append(values)
  elif tag=='INIT_BIN':current['bins'].append(values)
  elif tag=='INIT_INTEGRAL':current['integral']=values[0]
  elif tag=='INIT_OVERLAP':current['overlap']=values
  elif tag=='INIT_END':records.append(current);current=None
 assert current is None and records and a.baseline
 for r in records:assert len(r['points'])==20*r['samples'] and len(r['draws'])==120*r['samples'] and len(r['bins'])==20
 provenance=dict(hepmc_byte_identical=sha(a.reference/'events.hepmc')==sha(a.baseline),baseline_hepmc_sha256=sha(a.baseline),artifacts={name:sha(a.reference/name) for name in ['pythia.f','trace.f90','reference','params.dat','medium.dat','run.log','instrumentation.patch']})
 evidence='live-reference-trace'
payload=''
for r in records:
 for row in [[r['samples'],*r['config']],[len(r['draws']),*r['draws']]]:payload+=' '.join(format(v,'.17g') for v in row)+'\n'
run=subprocess.run([str(a.cpp.resolve()),str(a.width_config.resolve()),format(a.alpha_s,'.17g')],input=payload,capture_output=True,text=True,timeout=180);assert run.returncode==0,run.stderr[-2000:]
native=[];current=dict(points=[],bins=[],overlap=None)
for line in run.stdout.splitlines():
 t=line.split();v=list(map(float,t[1:]))
 if t[0]=='SAMPLE':current['points'].append(v)
 elif t[0]=='BIN':current['bins'].append(v)
 elif t[0]=='INTEGRAL':current['integral']=v[0];current['draws']=int(v[1])
 elif t[0]=='OVERLAP':current['overlap']=v
 elif t[0]=='END':native.append(current);current=dict(points=[],bins=[],overlap=None)
 else:raise AssertionError(line)
assert len(native)==len(records);groups={};failures=[]
for index,(cpp,ref) in enumerate(zip(native,records)):
 def compare(name,x,y,exact=False):
  g=groups.setdefault(name,dict(values=0,max_absolute_error=0.,max_relative_error=0.));g['values']+=1;g['max_absolute_error']=max(g['max_absolute_error'],abs(x-y));g['max_relative_error']=max(g['max_relative_error'],abs(x-y)/max(abs(y),1e-300))
  if not math.isfinite(x) or not math.isfinite(y) or (x!=y if exact else not math.isclose(x,y,rel_tol=2e-12,abs_tol=2e-13)):
   if len(failures)<30:failures.append(dict(pass_index=index,group=name,cpp=x,fortran=y))
 assert len(cpp['points'])==len(ref['points']) and len(cpp['bins'])==20
 for x,y in zip(cpp['points'],ref['points']):
  assert len(x)==len(y)==5
  for j,(u,v) in enumerate(zip(x,y)):compare('cross-sections' if j==4 else 'sampled-kinematics',u,v)
 for x,y in zip(cpp['bins'],ref['bins']):
  for j,(u,v) in enumerate(zip(x,y)):compare('bin-counts' if j<2 else 'bin-sums',u,v,j<2)
 compare('integral',cpp['integral'],ref['integral']);compare('draw-count',cpp['draws'],len(ref['draws']),True)
 assert (cpp['overlap'] is None)==(ref['overlap'] is None)
 if ref['overlap']:
  for u,v in zip(cpp['overlap'],ref['overlap']):compare('overlap-normalization',u,v)
report=dict(scope='native PYMULT integration passes and double-Gaussian normalization from recorded settings/randomness; no adaptive retries or secondary initializer',passes=len(records),sampled_points=sum(len(r['points']) for r in records),random_draws=sum(len(r['draws']) for r in records),integrals=[r['integral'] for r in native],configurations=[dict(beams=r['config'][10:12],screening_squared=r['config'][1],scale_shift=r['config'][5],core_fraction=r['config'][2],core_radius=r['config'][3],sigma_nd=r['config'][4],samples_per_bin=r['samples'],coefficients=r['config'][22:27]) for r in records],groups=groups,failures=failures,reference_evidence=evidence,provenance=provenance,input_sha256=hashlib.sha256(payload.encode()).hexdigest(),cpp_sha256=sha(a.cpp),width_config_sha256=sha(a.width_config),alpha_s_mz=a.alpha_s,relative_tolerance=2e-12,absolute_tolerance=2e-13)
a.output.write_text(json.dumps(report,indent=2)+'\n');assert provenance['hepmc_byte_identical'] and not failures,failures[:5]
if a.write_fixture:
 a.write_fixture.parent.mkdir(parents=True,exist_ok=True);a.write_fixture.write_bytes(gzip.compress(json.dumps(dict(schema=1,records=records,provenance=provenance),separators=(',',':')).encode(),mtime=0))
print('PASS',report['sampled_points'],'points;',report['random_draws'],'draws; integrals',report['integrals'])
