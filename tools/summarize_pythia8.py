"""Summarize native Pythia8 boundary artifacts without claiming JEWEL parity."""
import argparse,hashlib,json,math
from collections import Counter
from pathlib import Path
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('snapshots',type=Path);parser.add_argument('boundary',type=Path)
parser.add_argument('pythia_root',type=Path);parser.add_argument('output',type=Path)
a=parser.parse_args()
def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()
rows=[json.loads(line) for line in a.snapshots.read_text().splitlines()]
header,summary=rows[0],rows[-1]
assert header['type']=='run' and header['schema']=='jewel-hard-input-v1' and summary['type']=='summary'
events=[r for r in rows if r['type']=='event'];failures=[r for r in rows if r['type']=='generation_failure']
assert len(events)==summary['returned'] and len(failures)==summary['failed']
assert len(events)+len(failures)==header['attempts_requested']==summary['attempts']
assert sorted(r['attempt'] for r in events+failures)==list(range(1,summary['attempts']+1))
maximum=0.;copied=0
for e in events:
 p=e['particles'];assert [x['id'] for x in p]==list(range(1,len(p)+1))
 for i in e['hard_outgoing']:assert abs(p[i-1]['status'])==23
 for i in e['hard_final']:assert p[i-1]['final']
 copied+=sum(i!=j for i,j in zip(e['hard_outgoing'],e['hard_final']))
 total=[sum(x['momentum'][i] for x in p if x['final']) for i in range(4)]
 delta=max(abs(total[i]-p[0]['momentum'][i]) for i in range(4));assert math.isfinite(delta) and delta<1e-8*max(1,p[0]['momentum'][3])
 maximum=max(maximum,delta)
metadata=[]
for folder in ['xmldoc','pdfdata','tunes']:
 for p in sorted((a.pythia_root/'share/Pythia8'/folder).rglob('*')):
  if p.is_file():metadata.append([str(p.relative_to(a.pythia_root)),digest(p)])
libraries=[p for name in ['libpythia8.dylib','libpythia8.so','libpythia8.a'] if (p:=a.pythia_root/'lib'/name).exists()]
report={'scope':'Pythia8 input and hadronization adapters only; no JEWEL shower or Fortran physics equivalence',
 'full_goal_validated':False,'input_header':header,'input_statistics':summary,
 'hard_outgoing_partons':sum(len(e['hard_outgoing']) for e in events),'hard_legs_with_final_copy':copied,
 'max_input_final_state_residual_GeV':maximum,
 'process_counts':dict(sorted(Counter(e['process_code'] for e in events).items())),
 'junction_events':sum(bool(e['junctions']) for e in events),
 'hadronization':json.loads(a.boundary.read_text()),
 'provenance':{'snapshot_sha256':digest(a.snapshots),'boundary_sha256':digest(a.boundary),
  'pythia_header_sha256':digest(a.pythia_root/'include/Pythia8/Pythia.h'),
  'libraries':{p.name:digest(p) for p in libraries},
  'data_files':len(metadata),'data_manifest_sha256':hashlib.sha256(json.dumps(metadata,separators=(',',':')).encode()).hexdigest()}}
a.output.write_text(json.dumps(report,indent=2,allow_nan=False)+'\n')
print('Wrote',a.output)
