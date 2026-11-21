"""Experimental full-input runner must preserve settings, ledger and replay."""
import collections,json,math,subprocess,sys,tempfile
from pathlib import Path
runner,probe,hadron_runner=[str(Path(p).resolve()) for p in sys.argv[1:]]
with tempfile.TemporaryDirectory() as tmp:
    root=Path(tmp);output=root/'sample.jsonl';fixtures=root/'fixtures'
    subprocess.run([runner,str(output),'1','2600417','vacuum',str(fixtures),'--experimental-isr-mpi'],check=True,capture_output=True,timeout=90)
    rows=[json.loads(line) for line in output.open()]
    assert rows[0]['experimental_isr_mpi'] is True
    assert rows[0]['mpi_channel_policy']=='outgoing-flavour'
    assert rows[0]['medium_backend']=='jewel-vacuum' and rows[0]['table_lifetime_fm']==0
    settings=rows[0]['pythia_settings']
    assert 'PartonLevel:ISR = on' in settings and 'PartonLevel:MPI = on' in settings
    event=rows[1];assert event['type']=='event',event
    assert event['event_lifetime_fm']==0 and event['thermal_references']==[]
    assert all(p['production_fm']==[0,0,0,0] for p in event['initial_hard'])
    assert (fixtures/'1.input').read_text().startswith('VACUUM\n')
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
    # Exercise the real cascade -> explicit junction -> Pythia8 boundary.
    # Includes the former quark/gluon MPI channel mismatch at attempt six.
    hadron_output=root/'hadrons.jsonl'
    subprocess.run([hadron_runner,str(hadron_output),'6','2600417','vacuum','--experimental-isr-mpi'],check=True,capture_output=True,timeout=90)
    hadrons=[json.loads(line) for line in hadron_output.open()]
    assert hadrons[-1]['attempts']==6 and hadrons[-1]['accepted']==6
    failures=[r for r in hadrons if r['type']=='evolution_failure']
    assert failures==[]
    assert hadrons[-1]['accepted_weight']+hadrons[-1]['failed_weight']==hadrons[-1]['generator_weight_sum']
    junction_events=0
    for e in hadrons:
        if e['type']!='event':continue
        junction_events+=bool(e['hadronization_junctions'])
        counts=collections.defaultdict(lambda:[0,0])
        for p in e['hadronization_input']:
            for side,key in enumerate(['colour','anticolour']):
                if p[key]:counts[p[key]][side]+=1
        for j in e['hadronization_junctions']:
            assert j['kind'] in [1,2]
            for tag in j['colours']:counts[tag][1 if j['kind']==1 else 0]+=1
        assert all(v==[1,1] for v in counts.values())
        final=[p for p in e['hadronized_particles'] if p['final']]
        assert final
        for component in range(4):
            before=math.fsum(p['momentum'][component] for p in e['hadronization_input'])
            after=math.fsum(p['momentum'][component] for p in final)
            assert math.isclose(before,after,rel_tol=1e-9,abs_tol=1e-7),(before,after)
    assert junction_events==3
    for executable,options in [(runner,['--experimental-isr-mpi','--experimental-isr-mpi']),(hadron_runner,['--experimental-isr-mpi','--hadrotype','1']),(runner,['--centrality','0','10'])]:
        invalid=root/'invalid.jsonl'
        result=subprocess.run([executable,str(invalid),'1','2600417','vacuum',*options],capture_output=True,timeout=10)
        assert result.returncode!=0 and not invalid.exists()
print('Experimental ISR/MPI settings, complete ledger, native replay and option guards passed')
