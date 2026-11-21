"""Audit native HADROTYPE=1 output, including synthetic endpoint accounting."""
import argparse,collections,hashlib,json,math,sys
from pathlib import Path
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('output',type=Path);parser.add_argument('samples',nargs='+',type=Path);parser.add_argument('--generator',type=Path)
args=parser.parse_args();output=args.output;reports=[]
for filename in args.samples:
 path=Path(filename);rows=[json.loads(l) for l in path.read_text().splitlines()]
 header,summary=rows[0],rows[-1]
 assert header['hadrotype']==1 and summary['type']=='summary'
 assert header['minimum_string_policy']['endpoint_energy_gev']==10
 events=[r for r in rows if r['type']=='event']
 assert len(events)==summary['accepted']
 assert len(rows)-2==summary['attempts']
 synthetic=split_halves=draws=0;maximum=0.
 for e in events:
  inputs=e['hadronization_input'];sources=e['hadronization_native_sources']
  native={p['id']:p for p in e['particles']}
  assert len(inputs)==len(sources) and [p['id'] for p in inputs]==list(range(1,len(inputs)+1))
  colours=collections.Counter(p['colour'] for p in inputs if p['colour'])
  antis=collections.Counter(p['anticolour'] for p in inputs if p['anticolour'])
  assert colours==antis and all(n==1 for n in colours.values())
  for p,source in zip(inputs,sources):
   assert p['pdg']!=0 and p['momentum'][3]>0
   if source is None:
    synthetic+=1
    assert p['mass']==0 and p['momentum'] in [[0,0,10,10],[0,0,-10,10]]
   else:
    q=native[source]
    if q['pdg']==21 and abs(p['pdg'])==1:
     split_halves+=1;scale=p['momentum'][3]/q['momentum'][3]
     assert all(math.isclose(a,b*scale,rel_tol=2e-12,abs_tol=2e-13) for a,b in zip(p['momentum'],q['momentum']))
     assert math.isclose(p['mass'],q['mass']*scale,rel_tol=2e-12,abs_tol=2e-13)
    else:assert p['pdg']==q['pdg'] and p['momentum']==q['momentum'] and p['mass']==q['mass']
  final=[p for p in e['hadronized_particles'] if p['final']]
  assert all(not p['colour'] and not p['anticolour'] for p in final)
  total=[math.fsum(p['momentum'][i] for p in inputs) for i in range(4)]
  after=[math.fsum(p['momentum'][i] for p in final) for i in range(4)]
  for a,b in zip(total,after):
   maximum=max(maximum,abs(a-b));assert abs(a-b)<=1e-8*max(1,total[3])
  draws+=e['string_draws']
 reports.append(dict(mode=header['mode'],summary=summary,synthetic_endpoints=synthetic,
  generated_quark_halves=split_halves,string_draws=draws,max_hadronization_closure_gev=maximum,
  failures=dict(collections.Counter(r.get('stage','generation')+': '+r.get('reason','generation failure') for r in rows if 'failure' in r['type'])),
  input_sha256=hashlib.sha256(path.read_bytes()).hexdigest()))
report=dict(generator_sha256=None if args.generator is None else hashlib.sha256(args.generator.read_bytes()).hexdigest(),scope='native minimum-string export and Pythia8 hadronization; not Pythia6 hadron-level parity',reports=reports)
output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
