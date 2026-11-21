"""Experimental full-input runner must preserve settings, ledger and replay."""
import json,subprocess,sys,tempfile
from pathlib import Path
runner,probe,hadron_runner=[str(Path(p).resolve()) for p in sys.argv[1:]]
with tempfile.TemporaryDirectory() as tmp:
    root=Path(tmp);output=root/'sample.jsonl';fixtures=root/'fixtures'
    subprocess.run([runner,str(output),'1','2600417','vacuum',str(fixtures),'--experimental-isr-mpi'],check=True,capture_output=True,timeout=90)
    rows=[json.loads(line) for line in output.open()]
    assert rows[0]['experimental_isr_mpi'] is True
    settings=rows[0]['pythia_settings']
    assert 'PartonLevel:ISR = on' in settings and 'PartonLevel:MPI = on' in settings
    event=rows[1];assert event['type']=='event',event
    assert rows[-1]['accepted']==1 and rows[-1]['accepted_weight']==event['weight']
    assert all(p['status'] not in [1,2] for p in event['particles'])
    replay=subprocess.run([probe],input=(fixtures/'1.input').read_text(),text=True,capture_output=True,check=True,timeout=90)
    replay_rows=[list(map(float,line.split())) for line in replay.stdout.splitlines()]
    header=replay_rows[0];assert header[0]==len(event['particles']) and header[4]==event['draws'] and header[5:7]==[0,0]
    for native,reference in zip(event['particles'],replay_rows[1:]):
        assert native['momentum']==reference[5:9] and native['mass']==reference[9]
    # Covers imported-fraction and zero-CM collision-roundoff regressions.
    medium_output=root/'medium.jsonl'
    subprocess.run([runner,str(medium_output),'20','2600417','medium','--experimental-isr-mpi'],check=True,capture_output=True,timeout=90)
    medium=[json.loads(line) for line in medium_output.open()]
    assert medium[-1]['accepted']==20 and medium[-1]['evolution_failures']==0
    assert [r['attempt'] for r in medium if r['type']=='event']==list(range(1,21))
    for executable,options in [(runner,['--experimental-isr-mpi','--experimental-isr-mpi']),(hadron_runner,['--experimental-isr-mpi'])]:
        invalid=root/'invalid.jsonl'
        result=subprocess.run([executable,str(invalid),'1','2600417','vacuum',*options],capture_output=True,timeout=10)
        assert result.returncode!=0 and not invalid.exists()
print('Experimental ISR/MPI settings, complete ledger, native replay and option guards passed')
