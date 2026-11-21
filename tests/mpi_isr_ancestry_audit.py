"""Audit per-subsystem ISR ownership and source-stage spacelike reconstruction."""
from decimal import Decimal, localcontext
import hashlib
import itertools
import json
import math
from pathlib import Path
import sys

reports=[]
assert len(sys.argv)>=4 and len(sys.argv[2:])%2==0
for current,prior in zip(sys.argv[2::2],sys.argv[3::2]):
    events=systems=branches=negative=0
    maximum_q2_error=0.
    with open(current) as a,open(prior) as b:
        for line,old_line in itertools.zip_longest(a,b):
            assert line is not None and old_line is not None
            event,old=json.loads(line),json.loads(old_line)
            if event['type']!='event':
                assert event==old
                continue
            chains_by_system=event.pop('subsystem_isr_chains')
            assert event==old, 'ancestry capture changed existing event content'
            events+=1
            assert len(chains_by_system)==len(event['subsystems'])
            particles={p['id']:p for p in event['particles']}
            all_incoming=set();all_emissions=set()
            ledger=dict(zip(event['isr_emissions'],event['isr_final']))
            for system,chains in zip(event['subsystems'],chains_by_system):
                systems+=1
                roots=particles[system['process_outgoing'][0]]['mothers']
                assert len(chains)==2 and chains[0]['beam']!=chains[1]['beam']
                for side,chain in enumerate(chains):
                    assert chain['momentum_stage']=='before_beam_energy_repair'
                    assert chain['hard_incoming']==roots[side]
                    lines=chain['backward_lines']
                    assert lines[0]==roots[side] and lines[-1]==system['incoming'][side]
                    assert len(lines)==len(set(lines)) and not all_incoming.intersection(lines)
                    all_incoming.update(lines)
                    beam=particles[chain['beam']]
                    assert abs(beam['status'])==12
                    assert abs(particles[lines[-1]]['status'])==61
                    expected={}
                    for child_id,parent_id in zip(lines,lines[1:]+[chain['beam']]):
                        child,parent=particles[child_id],particles[parent_id]
                        assert child['mothers']==[parent_id] and child_id in parent['daughters']
                        status=abs(parent['status'])
                        assert status in (12,41,42,61)
                        if status==41:
                            assert len(parent['daughters'])==2
                            emitted=next(i for i in parent['daughters'] if i!=child_id)
                            assert particles[emitted]['mothers']==[parent_id]
                            assert abs(particles[emitted]['status'])==43
                            expected[emitted]=(parent_id,child_id)
                        elif status!=12:
                            assert parent['daughters']==[child_id] and parent['pdg']==child['pdg']
                    assert len(chain['branches'])==len(expected)
                    for branch in chain['branches']:
                        emitted=branch['emitted'];assert emitted in expected and emitted not in all_emissions
                        all_emissions.add(emitted);branches+=1
                        assert expected[emitted]==(branch['mother'],branch['daughter'])
                        assert branch['emitted_final']==ledger[emitted] and ledger[emitted] in system['outgoing_final']
                        def staged(i):
                            p=particles[i]
                            return [*p['momentum'][:3],p['energy_before_beam_repair']]
                        m,s=staged(branch['mother']),staged(emitted)
                        q=[x-y for x,y in zip(m,s)]
                        assert q==branch['spacelike_daughter']
                        negative+=q[3]<0
                        with localcontext() as context:
                            context.prec=80
                            d=[Decimal.from_float(v) for v in q]
                            exact=sum(v*v for v in d[:3])-d[3]*d[3]
                            residual=abs(Decimal.from_float(branch['virtuality_squared'])-exact)
                        maximum_q2_error=max(maximum_q2_error,float(residual))
                        assert math.isfinite(branch['virtuality_squared']) and branch['virtuality_squared']>0
                        assert float(residual)<=32*math.ulp(max(1.,sum(v*v for v in q)))
            assert all_emissions==set(ledger)=={i for i,p in particles.items() if abs(p['status'])==43}
    reports.append(dict(path=current,sha256=hashlib.sha256(Path(current).read_bytes()).hexdigest(),
        prior_sha256=hashlib.sha256(Path(prior).read_bytes()).hexdigest(),events=events,
        subsystems=systems,branches=branches,negative_stage_local_energies=negative,
        max_q2_decimal_residual_gev2=maximum_q2_error))
Path(sys.argv[1]).write_text(json.dumps(dict(scope='subsystem ISR ancestry only; no assigned JEWEL times',reports=reports),indent=2)+'\n')
print(json.dumps(reports,indent=2))
