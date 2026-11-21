"""Compare public-hook fraction observations with isolated Pythia source traces."""
import hashlib
import itertools
import json
from pathlib import Path
import sys
reports=[]
assert len(sys.argv)>=5 and len(sys.argv[2:])%3==0
for snapshot,prior,trace in zip(sys.argv[2::3],sys.argv[3::3],sys.argv[4::3]):
    source={}
    for line in open(trace):
        r=json.loads(line)
        if 'inputs' in r:
            key=(r['attempt'],r['emitted']);assert key not in source;source[key]=r
    values=events=0;seen=set();max_z=0.;max_q2={'light_or_massless':0.,'massive_fallback':0.}
    with open(snapshot) as a,open(prior) as b:
        for line,old_line in itertools.zip_longest(a,b):
            assert line is not None and old_line is not None
            e,old=json.loads(line),json.loads(old_line)
            if e['type']!='event':
                assert e==old
                continue
            events+=1
            for key in ['subsystem_isr_frames','subsystem_isr_frame_error']:
                e.pop(key);old.pop(key)
            fractions=e.pop('isr_beam_fractions')
            assert e==old, 'fraction observation changed pre-existing event data'
            particles={p['id']:p for p in e['particles']}
            branches={b['emitted']:b for chains in e['subsystem_isr_chains'] for chain in chains for b in chain['branches']}
            for f in fractions:
                key=(e['attempt'],f['emitted']);assert key not in seen;seen.add(key);r=source[key]
                assert (f['subsystem'],f['mother'],f['recoiler'])==(r['system'],r['mother'],r['recoiler'])
                assert f['before']+f['after']==r['inputs'][3:7];values+=4
                assert f['before'][0]*f['before'][1]*f['collision_s_gev2']==r['inputs'][2]
                side=0 if f['after'][0]>f['before'][0] else 1
                assert f['after'][side]>f['before'][side] and f['after'][1-side]==f['before'][1-side]
                z=f['before'][side]/f['after'][side];max_z=max(max_z,abs(z-r['inputs'][1]))
                sister=particles[f['emitted']]
                if sister['mass']==0 or abs(sister['pdg'])<=3:
                    q2=sister['scale']**2/(1-z);group='light_or_massless'
                else:q2=branches[f['emitted']]['virtuality_squared'];group='massive_fallback'
                max_q2[group]=max(max_q2[group],abs(q2-r['inputs'][0]))
            assert {f['emitted'] for f in fractions}==set(branches)
    assert seen==set(source)
    reports.append(dict(events=events,branches=len(seen),exact_fraction_values=values,
        source_subsystem_mass_squared_exact=True,max_reconstructed_z_absolute_error=max_z,
        max_reconstructed_q2_absolute_error_gev2=max_q2,
        sha256={name:hashlib.sha256(Path(path).read_bytes()).hexdigest() for name,path in [('snapshot',snapshot),('prior',prior),('trace',trace)]}))
Path(sys.argv[1]).write_text(json.dumps(dict(scope='ISR public-hook beam fractions versus exact source inputs',reports=reports),indent=2)+'\n')
print(json.dumps(reports,indent=2))
