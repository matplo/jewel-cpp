"""Validate import against independent HepMC2 text and corrupt-file cases."""
import json
import re
import tempfile
from pathlib import Path
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from import_reference_hepmc import import_events, discard_ledger
source,cards,medium=map(Path,sys.argv[1:])
text=source.read_text();expected=[]
for line in text.splitlines():
    fields=line.split()
    if not fields:continue
    if fields[0]=='E':expected.append(dict(number=int(fields[1]),weight=float(fields[-1]),particles=[]))
    elif fields[0]=='P' and int(fields[8])==1:
        expected[-1]['particles'].append((int(fields[2]),list(map(float,fields[3:7]))))
with tempfile.TemporaryDirectory() as directory:
    directory=Path(directory);output=directory/'events.jsonl'
    import_events(source,cards,medium,'vacuum',output)
    records=[json.loads(line) for line in output.read_text().splitlines()]
    assert records[0]['denominator_scope']=='written-events'
    assert records[-1]['failed_attempts_known'] is False
    assert records[-1]['generation_failures'] is None
    assert len(expected)==len(records)-2
    for raw,record in zip(expected,records[1:-1]):
        assert raw['number']==record['reference_event_number'] and raw['weight']==record['weight']
        assert raw['particles']==[(p['pdg'],p['momentum']) for p in record['final_particles']]
    # Add one explicitly logged lost attempt between written events. Particle
    # contents stay identical; only the original attempt association changes.
    ledger_cards=directory/'ledger.dat'
    ledger_cards.write_text(re.sub(r'(?m)^NEVENT\s+\d+',f'NEVENT {len(expected)+1}',cards.read_text()))
    settings=dict(line.split(None,1) for line in ledger_cards.read_text().splitlines() if line.strip())
    log=directory/'jewel.log'
    log_text=f"NEVENT = {len(expected)+1}\nNJOB = {settings['NJOB']}\ndiscard event 2\ntotal number of discarded events: 1\nnumber of events for which conversion to hepmc failed: 0\n"
    log.write_text(log_text)
    ledger_output=directory/'ledger.jsonl'
    import_events(source,ledger_cards,medium,'vacuum',ledger_output,log)
    ledger=[json.loads(line) for line in ledger_output.read_text().splitlines()]
    assert ledger[0]['denominator_scope']=='attempted-events'
    assert ledger[2]['type']=='reference_failure' and ledger[2]['attempt']==2 and ledger[2]['weight']==1
    assert ledger[-1]['attempts']==len(expected)+1 and ledger[-1]['accepted']==len(expected)
    assert ledger[-1]['generator_weight_sum']==len(expected)+1
    accepted=[r for r in ledger if r['type']=='event']
    assert [r['attempt'] for r in accepted]==[1]+list(range(3,len(expected)+2))
    assert [r['final_particles'] for r in accepted]==[r['final_particles'] for r in records[1:-1]]
    for bad in [log_text.replace('discard event 2',''),log_text.replace('NEVENT =','NEVENT_BAD ='),
                log_text.replace('failed: 0','failed: 1'),log_text.replace('discard event 2','discard event 0'),
                log_text+'discard event 2\n',log_text.replace('discard event 2','discard event 99999')]:
        log.write_text(bad)
        try:discard_ledger(settings,log)
        except ValueError:pass
        else:raise AssertionError('accepted inconsistent discard ledger')
    log.write_text(log_text)
    for key,value in [('WEIGHTED','T'),('ISOCHANNEL','XX')]:
        try:discard_ledger({**settings,key:value},log)
        except ValueError:pass
        else:raise AssertionError('accepted unsupported ledger policy')
    original=output.read_bytes()
    try:import_events(source,cards,medium,'vacuum',output)
    except ValueError:pass
    else:raise AssertionError('overwrote existing output')
    assert output.read_bytes()==original
    for name,bad_text,bad_cards in [
        ('truncated',text.replace('HepMC::IO_GenEvent-END_EVENT_LISTING',''),cards.read_text()),
        ('units',text.replace('U GEV MM','U MEV MM'),cards.read_text()),
        ('count',text,re.sub(r'(?m)^NEVENT\s+\d+',f'NEVENT {len(expected)+1}',cards.read_text())),
        ('dummy',text,cards.read_text().replace('WRITEDUMMIES F','WRITEDUMMIES T')),
        ('duplicate-card',text,cards.read_text()+'\nNEVENT 3\n'),
        ('duplicate-event',text.replace(f'E {expected[1]["number"]} ',f'E {expected[0]["number"]} ',1),cards.read_text()),
    ]:
        fixture=directory/(name+'.hepmc');fixture.write_text(bad_text)
        card=directory/(name+'.dat');card.write_text(bad_cards)
        target=directory/(name+'.jsonl')
        try:import_events(fixture,card,medium,'vacuum',target)
        except ValueError:pass
        else:raise AssertionError('accepted invalid '+name)
        assert not target.exists()
    print(f'{len(expected)} events / {sum(len(e["particles"]) for e in expected)} particles match independent text exactly; corruption/overwrite checks pass')
