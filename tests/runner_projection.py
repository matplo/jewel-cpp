"""Projection wiring against unchanged source on actual subtracted event inputs."""
from concurrent.futures import ThreadPoolExecutor
import json,math,subprocess,sys,tempfile
from pathlib import Path
runner,parton=[str(Path(p).resolve()) for p in sys.argv[1:3]]
reference=str(Path(sys.argv[3]).resolve()) if len(sys.argv)>3 else None
with tempfile.TemporaryDirectory() as directory:
 root=Path(directory)
 def run(mode):
  output=root/f'{mode}.jsonl';extra=[] if mode is None else ['--put-on-shell',str(mode)]
  subprocess.run([runner,str(output),'1','2601001','medium','--subtraction-radius','.4',*extra],capture_output=True,check=True,timeout=90)
  return [json.loads(line) for line in output.read_text().splitlines()]
 with ThreadPoolExecutor(max_workers=2) as pool:samples=list(pool.map(run,[None,0,1,2]))
 assert samples[0]==samples[1],'explicit zero changes default output'
 baseline=samples[0];assert baseline[1]['type']=='event',baseline[1].get('reason')
 for mode,sample in zip([1,2],samples[2:]):
  event=sample[1];assert event['type']=='event',event.get('reason')
  assert sample[0]['subtraction']['put_on_shell']==mode
  assert sample[0]['projection_mass_source']=='Pythia8 ParticleData::m0'
  for key,value in baseline[1].items():assert event[key]==value,('preprojection field changed',mode,key)
  assert sample[-1]==baseline[-1]
  before,after=event['subtracted_particles'],event['projected_particles'];assert len(before)==len(after)
  for old,new in zip(before,after):
   for key in ['id','source_id','pdg','final','thermal_residual']:assert old[key]==new[key]
   assert new['mass']==new['nominal_mass']>=0
   p=new['momentum'];assert all(math.isfinite(v) for v in p)
   assert math.isclose(p[3]**2-sum(v*v for v in p[:3]),new['mass']**2,rel_tol=1e-10,abs_tol=1e-10*max(1,p[3]**2))
   if new['pdg']==22:assert new['nominal_mass']==0
   if abs(new['pdg'])==211:assert math.isclose(new['nominal_mass'],.13957,abs_tol=1e-12)
   if mode==1:assert p[:3]==old['momentum'][:3]
  for label,records in [('total_before',before),('total_after',after)]:
   for col in range(4):assert math.isclose(event['projection'][label][col],sum(p['momentum'][col] for p in records),rel_tol=1e-12,abs_tol=1e-12)
  if mode==2:
   assert event['projection']['massless_converged'] and event['projection']['massive_converged']
   assert not event['projection']['insufficient_energy']
  if reference:
   payload=f'{len(before)} {mode} 0\n'+''.join(' '.join(map(str,[*old['momentum'],old['mass'],new['nominal_mass']]))+'\n' for old,new in zip(before,after))
   result=subprocess.run([reference],input=payload,text=True,capture_output=True,check=True,timeout=30)
   rows=result.stdout.splitlines();assert int(rows[0])==len(after);maximum=0
   for row,p in zip(rows[1:],after):
    values=list(map(float,row.split()));assert len(values)==5
    for a,b in zip([*p['momentum'],p['mass']],values):
     assert math.isfinite(b) and math.isclose(a,b,rel_tol=2e-12,abs_tol=2e-13),(mode,a,b)
     maximum=max(maximum,abs(a-b))
   print(json.dumps(dict(mode=mode,particles=len(after),values=5*len(after),max_absolute_error=maximum,projection=event['projection'])))
 for index,extra in enumerate([['--put-on-shell'],['--put-on-shell','3'],['--put-on-shell','-1'],['--put-on-shell','1.0'],['--put-on-shell','1','--put-on-shell','2']]):
  path=root/f'invalid{index}.jsonl';r=subprocess.run([runner,str(path),'1','2601001','medium','--subtraction-radius','.4',*extra],capture_output=True,timeout=10)
  assert r.returncode and not path.exists()
 for binary,extra in [(runner,['--put-on-shell','1']),(parton,['--subtraction-radius','.4','--put-on-shell','1'])]:
  path=root/'unsupported.jsonl';r=subprocess.run([binary,str(path),'1','2601001','medium',*extra],capture_output=True,timeout=10)
  assert r.returncode and not path.exists()
print('Projection preserves earlier records, explicit zero is unchanged, nominal masses and routing metadata validate')
