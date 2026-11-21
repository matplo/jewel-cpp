"""CLI policy and default reproducibility for the hard-input adapter."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile
exe=sys.argv[1]
with tempfile.TemporaryDirectory() as directory:
    directory=Path(directory)
    results=[]
    for name,options in [('default',[]),('off',['--isr','off']),('on',['--isr','on'])]:
        output=directory/(name+'.jsonl')
        run=subprocess.run([exe,str(output),'5','2600317',*options],capture_output=True,text=True)
        assert run.returncode==0,run.stderr
        results.append([json.loads(line) for line in output.read_text().splitlines()])
    assert results[0]==results[1]
    assert all(not e['initial_state_radiation'] and not e['isr_emissions'] for e in results[0] if e['type']=='event')
    assert all(e['initial_state_radiation'] for e in results[2] if e['type']=='event')
    assert sum(len(e['isr_emissions']) for e in results[2] if e['type']=='event')>0
    for i,options in enumerate([['--isr'],['--isr','yes'],['--isr','on','--isr','off']]):
        output=directory/f'bad{i}.jsonl'
        run=subprocess.run([exe,str(output),'1','2600317',*options],capture_output=True,text=True)
        assert run.returncode!=0 and not output.exists()
print('ISR CLI default/off identity, on metadata and invalid-option checks passed')
