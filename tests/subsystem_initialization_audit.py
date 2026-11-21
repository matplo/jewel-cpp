"""Classify full-chain initialization outcomes against captured Pythia records."""
import collections, hashlib, json, math, sys
from pathlib import Path
sample, snapshot, output = sys.argv[1:]
rows = [json.loads(line) for line in Path(sample).open()]
header = rows[0]
assert header['initialize_subsystems'] is True
attempts = [r for r in rows if 'attempt' in r]
assert sorted(r['attempt'] for r in attempts) == list(range(1,header['attempts']+1))
events = {r['attempt']:r for r in rows if r['type']=='event'}
assert rows[-1]['completed']==len(events)
failures = [r for r in rows if r['type']=='failure']
classes = collections.Counter(); non_qcd = collections.Counter(); missing_tags=0; pairs=0; maximum=0.; covered=set()
for event in map(json.loads,Path(snapshot).open()):
    if event['type']=='run':
        assert event['settings']==header['settings'];continue
    if event['type']!='event' or event['attempt'] not in events:continue
    covered.add(event['attempt']); row=events[event['attempt']]; init=row['initialization']
    assert init['seed']==header['seed']*100000+event['attempt']
    particles={p['id']:p for p in event['particles']}; finals=[p for p in particles.values() if p['final']]
    if init['error'] is None:
        classes['initialized']+=1;assert init['pairs']==len(event['subsystems']);pairs+=init['pairs']
        assert math.isfinite(init['maximum_pair_p4_residual']);maximum=max(maximum,init['maximum_pair_p4_residual']);continue
    assert init['draws']==0
    if init['error']=='missing hard-parton colour partner':
        tags={tag for j in event['junctions'] for tag in j['colours']};missing=[]
        for p in finals:
            if p['id'] not in event['hard_final']:continue
            for own,other in [('colour','anticolour'),('anticolour','colour')]:
                if p[own] and not any(q[other]==p[own] for q in finals):missing.append(p[own])
        assert missing and all(tag in tags for tag in missing)
        classes['junction_colour_partner']+=1;missing_tags+=len(missing)
    elif init['error'] in ['subsystem initialization currently requires quark/gluon pairs','subsystem initialization requires quarks/gluons or inactive massless photons']:
        unsupported=[]
        for sub in event['subsystems']:
            for root in sub['process_outgoing']:
                seen=set()
                while not particles[root]['final']:
                    assert root not in seen;seen.add(root);assert len(particles[root]['daughters'])==1
                    root=particles[root]['daughters'][0]
                pdg=particles[root]['pdg']
                if pdg!=21 and not 1<=abs(pdg)<=5:unsupported.append(pdg)
        assert unsupported;non_qcd.update(unsupported);classes['non_qcd_pair']+=1
    else:raise AssertionError(init)
    failures.append(dict(attempt=event['attempt'],stage='initialization',**init))
assert covered==set(events)
report=dict(scope='Generated preparation/initialization diagnostic; not a Fortran full-event comparison',
 seed=header['seed'],attempts=header['attempts'],prepared_events=len(events),outcomes=dict(classes),
 initialized_pairs=pairs,junction_missing_tags=missing_tags,unsupported_pdgs=dict(non_qcd),
 maximum_pair_p4_residual_gev=maximum,failures=failures,
 hashes={p:hashlib.sha256(Path(p).read_bytes()).hexdigest() for p in [sample,snapshot,__file__]})
Path(output).write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({k:v for k,v in report.items() if k not in ['hashes','failures']},indent=2))
