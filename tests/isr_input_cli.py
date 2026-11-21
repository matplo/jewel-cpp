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
    for name,options in [('default',[]),('off',['--isr','off','--mpi','off']),('on',['--isr','on'])]:
        output=directory/(name+'.jsonl')
        run=subprocess.run([exe,str(output),'5','2600317',*options],capture_output=True,text=True)
        assert run.returncode==0,run.stderr
        results.append([json.loads(line) for line in output.read_text().splitlines()])
    assert results[0]==results[1]
    assert all(not e['initial_state_radiation'] and not e['isr_emissions'] for e in results[0] if e['type']=='event')
    assert all(e['initial_state_radiation'] for e in results[2] if e['type']=='event')
    assert sum(len(e['isr_emissions']) for e in results[2] if e['type']=='event')>0
    for isr in ['off','on']:
        output=directory/f'mpi-{isr}.jsonl'
        run=subprocess.run([exe,str(output),'5','2600317','--mpi','on','--isr',isr],capture_output=True,text=True)
        assert run.returncode==0,run.stderr
        events=[json.loads(line) for line in output.read_text().splitlines() if json.loads(line)['type']=='event']
        assert len(events)==5 and any(len(e['subsystems'])>1 for e in events)
        assert all(e['multiple_parton_interactions'] and e['isr_chains'] is None and e['isr_recoil_frames'] is None and e['primordial_recoil_frame'] is None for e in events)
    for i,options in enumerate([['--mpi'],['--mpi','yes'],['--mpi','on','--mpi','off'],['--isr'],['--isr','yes'],['--isr','on','--isr','off']]):
        output=directory/f'bad{i}.jsonl'
        run=subprocess.run([exe,str(output),'1','2600317',*options],capture_output=True,text=True)
        assert run.returncode!=0 and not output.exists()
print('ISR CLI default/off identity, on metadata and invalid-option checks passed')
