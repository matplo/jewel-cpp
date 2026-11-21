"""Validate import against independent HepMC2 text and corrupt-file cases."""
import json
import re
import tempfile
from pathlib import Path
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from import_reference_hepmc import import_events
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
