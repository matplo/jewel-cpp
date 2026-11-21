"""PI0DEC runner wiring and final-state particle content."""
import json,subprocess,sys,tempfile
from pathlib import Path
events,partons=sys.argv[1:]
with tempfile.TemporaryDirectory() as tmp:
 root=Path(tmp)
 for mode in ['on','off']:
  out=root/(mode+'.jsonl')
  subprocess.run([events,str(out),'3','2600417','vacuum','--pi0-decay',mode],check=True,capture_output=True,text=True,timeout=120)
  rows=[json.loads(line) for line in out.read_text().splitlines()]
  assert '111:mayDecay = '+mode in rows[0]['hadronizer_settings']
  accepted=[r for r in rows if r['type']=='event'];assert len(accepted)==3
  pions=sum(p['final'] and p['pdg']==111 for r in accepted for p in r['hadronized_particles'])
  assert (pions==0 if mode=='on' else pions>0)
 for i,(exe,args) in enumerate([(events,['--pi0-decay','bad']),(events,['--pi0-decay','on','--pi0-decay','off']),(partons,['--pi0-decay','off'])]):
  out=root/('invalid'+str(i)+'.jsonl')
  result=subprocess.run([exe,str(out),'1','17','vacuum',*args],capture_output=True,text=True,timeout=30)
  assert result.returncode!=0 and not out.exists()
