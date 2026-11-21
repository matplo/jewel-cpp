"""Runner dispatch and default compatibility for the two JEWEL string modes."""
import json,subprocess,sys,tempfile
from pathlib import Path
runner=Path(sys.argv[1]).resolve()
with tempfile.TemporaryDirectory() as tmp:
 root=Path(tmp);samples=[]
 for name,options in [('default',[]),('zero',['--hadrotype','0']),('one',['--hadrotype','1'])]:
  output=root/(name+'.jsonl')
  subprocess.run([str(runner),str(output),'1','2601001','vacuum',*options],capture_output=True,check=True,timeout=90)
  samples.append([json.loads(l) for l in output.read_text().splitlines()])
 assert samples[0]==samples[1]
 header,event,summary=samples[2]
 assert header['hadrotype']==1 and 'minimum_string_policy' in header
 assert event['type']=='event' and summary['accepted']==1
 assert len(event['hadronization_native_sources'])==len(event['hadronization_input'])
 assert event['string_draws']>0
 for key in ['initial_hard','particles','draws','branch_calls','thermal_references']:
  assert event[key]==samples[0][1][key],key
 for options in [['--hadrotype'],['--hadrotype','2'],['--hadrotype','1.0'],['--hadrotype','-1'],['--hadrotype','1','--hadrotype','0']]:
  output=root/'invalid.jsonl';r=subprocess.run([str(runner),str(output),'1','2601001','vacuum',*options],capture_output=True,timeout=15)
  assert r.returncode!=0 and not output.exists()
print('Default output, pre-hadronization state, mode dispatch, provenance and invalid options pass')
