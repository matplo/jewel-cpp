"""Native subtraction wiring; optionally compare real-event momenta to stock helpers."""
import json,math,subprocess,sys,tempfile
from pathlib import Path
runner,parton=[str(Path(p).resolve()) for p in sys.argv[1:3]]
reference=str(Path(sys.argv[3]).resolve()) if len(sys.argv)>3 else None
with tempfile.TemporaryDirectory() as directory:
 root=Path(directory)
 def run(name,extra):
  path=root/(name+'.jsonl')
  subprocess.run([runner,str(path),'1','2601001','medium',*extra],check=True,capture_output=True,timeout=90)
  return [json.loads(line) for line in path.read_text().splitlines()]
 baseline=run('raw',[]);subtracted=run('sub',['--subtraction-radius','.4']);repeat=run('repeat',['--subtraction-radius','.4'])
 assert subtracted==repeat,'subtraction is not deterministic'
 assert baseline[1]['type']==subtracted[1]['type']=='event',(baseline[1]['type'],{k:v for k,v in subtracted[1].items() if k in ['type','stage','reason']})
 raw,event=baseline[1],subtracted[1]
 for key,value in raw.items():assert event[key]==value,('upstream event changed',key)
 assert baseline[-1]==subtracted[-1]
 settings=subtracted[0]['subtraction'];assert settings['radius']==.4 and settings['put_on_shell']==0
 assert settings['mass_policy']=='signed-invariant-mass'
 assert event['subtraction_seed']==settings['seed_base']+settings['seed_stride']
 source={p['id']:p for p in raw['hadronized_particles'] if p['final']}
 assert len(source)>0 and len(raw['thermal_references'])>0
 residuals=0
 for i,p in enumerate(event['subtracted_particles'],1):
  assert p['id']==i and p['final'] and all(math.isfinite(x) for x in [*p['momentum'],p['mass']])
  if p['thermal_residual']:
   residuals+=1;assert p['source_id']==0 and p['pdg'] in [211,-211,111]
  else:assert p['source_id'] in source and p['pdg']==source[p['source_id']]['pdg']
 assert event['subtraction_draws']==residuals
 if reference:
  selected=list(source.values());thermal=raw['thermal_references']
  payload=f'{len(selected)} {len(thermal)} .4 2600\n'
  payload+=''.join(' '.join(map(str,[p['pdg'],*p['momentum'],p['mass']]))+'\n' for p in selected)
  payload+=''.join(' '.join(map(str,t['momentum']))+'\n' for t in thermal)
  result=subprocess.run([reference],input=payload,text=True,capture_output=True,check=True,timeout=30)
  rows=result.stdout.splitlines();assert int(rows[0])==len(event['subtracted_particles'])
  repaired=0;maximum=0
  for p,line in zip(event['subtracted_particles'],rows[1:]):
   row=list(map(float,line.split()));assert len(row)==6
   if not p['thermal_residual']:assert row[0]==p['pdg']
   for a,b in zip(p['momentum'],row[1:5]):
    assert math.isfinite(b) and math.isclose(a,b,rel_tol=2e-12,abs_tol=2e-13),(a,b)
    maximum=max(maximum,abs(a-b))
   if math.isnan(row[5]):assert p['mass']<0;repaired+=1
   else:assert math.isclose(p['mass'],row[5],rel_tol=2e-12,abs_tol=2e-13)
  assert repaired==event['subtraction_spacelike_mass_fields']
  print(json.dumps(dict(reference_particles=len(rows)-1,max_momentum_error=maximum,spacelike_mass_fields=repaired,residual_thermal_particles=residuals)))
 for index,args in enumerate([['--subtraction-radius'],['--subtraction-radius','-1'],['--subtraction-radius','nan'],['--subtraction-radius','inf'],['--subtraction-radius','1x'],['--subtraction-radius','.4','--subtraction-radius','.3'],['--subtraction-radius','.4','--keep-recoils','off']]):
  path=root/f'invalid{index}.jsonl';r=subprocess.run([runner,str(path),'1','2601001','medium',*args],capture_output=True,timeout=10)
  assert r.returncode and not path.exists()
 path=root/'parton.jsonl';r=subprocess.run([parton,str(path),'1','2601001','medium','--subtraction-radius','.4'],capture_output=True,timeout=10)
 assert r.returncode and not path.exists()
print('Native subtraction preserves upstream records, provenance and deterministic output; invalid settings reject before output')
