"""Exercise the installed hepyy-utils converter on native HepMC3 output."""
import hashlib,importlib.metadata,json,sys,tempfile
from pathlib import Path
import numpy as np
import uproot
from hepyy_utils.jewel.convert import convert_hepmc_to_root
source,hepmc,report_path=map(Path,sys.argv[1:])
rows=[json.loads(l) for l in source.read_text().splitlines()];events=[r for r in rows if r['type']=='event']
header=rows[0]
key='projected_particles' if header.get('subtraction',{}).get('put_on_shell',0) else 'subtracted_particles' if header.get('subtraction') else 'hadronized_particles'
expected=[p for e in events for p in e[key] if p['final']]
with tempfile.TemporaryDirectory() as directory:
 output=Path(directory)/'converted.root'
 result=convert_hepmc_to_root(hepmc,output,progress=False)
 with uproot.open(output) as f:
  tracks=f['tracks'].arrays(library='np');info=f['event_info'].arrays(library='np')
  assert f['tracks'].num_entries==len(expected) and f['event_info'].num_entries==len(events)
  assert list(info['weight'])==[e['weight'] for e in events]
  assert list(info['jewel_event'])==[e['attempt'] for e in events]
  for i,name in enumerate(['px','py','pz','energy']):
   assert np.array_equal(tracks[name],np.array([p['momentum'][i] for p in expected],dtype=tracks[name].dtype))
report=dict(events=len(events),tracks=len(expected),weights_and_attempt_ids_preserved=True,
 momentum_preserved_at_root_storage_precision=True,collection=key,
 versions={name:importlib.metadata.version(name) for name in ['hepyy-utils','pyhepmc','uproot']},
 native_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),hepmc_sha256=hashlib.sha256(hepmc.read_bytes()).hexdigest())
report_path.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
