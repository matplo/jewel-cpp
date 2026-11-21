"""Exercise recoil cuts, explicit defaults and an active failing rescattering."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile

runner, probe = [str(Path(p).resolve()) for p in sys.argv[1:]]
with tempfile.TemporaryDirectory() as directory:
    root = Path(directory)
    outputs = {}
    settings = [('default', []), ('explicit', ['--recoil-rescattering','off','--recoil-cuts','0','0']),
                ('active', ['--recoil-rescattering','on','--recoil-cuts','0','0']),
                ('soft', ['--recoil-rescattering','on','--recoil-cuts','1000000','5'])]
    for name, options in settings:
        output, replay = root/(name+'.jsonl'), root/name
        subprocess.run([runner,str(output),'1','2601001','medium',str(replay),
                        '--centrality','0','10','--table-energy','jewel','--longitudinal-boost','on',*options],
                       check=True,capture_output=True,timeout=60)
        records = [json.loads(line) for line in output.read_text().splitlines()]
        outputs[name] = records
        header, event = records[:2]
        assert header['recoil_rescattering'] == (name in ('active','soft'))
        fixture = (replay/'1.input').read_text()
        finish = fixture.splitlines()[-4].split()
        assert float(finish[1]) == header['recoil_soft_cut_3T']
        assert float(finish[2]) == header['recoil_hard_cut_3T']
        assert bool(int(finish[4])) == header['recoil_rescattering']
        if name == 'active':
            assert event['type'] == 'evolution_failure' and event['stage'] == 'cascade'
            result = subprocess.run([probe],input=fixture,text=True,capture_output=True,check=True,timeout=60)
            replay_header = list(map(int,result.stdout.splitlines()[0].split()))
            assert replay_header[4] == event['draws'] and replay_header[6] != 0
        else:
            assert event['type'] == 'event', event
        if name == 'soft':
            statuses = {p['id']:p['status'] for p in event['particles']}
            assert all(statuses[p['id']] not in (3,13,22) for p in event['hadronization_input'])
            assert any(p['status']==13 for p in event['particles'])
    assert outputs['default'] == outputs['explicit']
    assert outputs['active'][1]['draws'] != outputs['default'][1]['draws']
    for options in [['--recoil-rescattering','true'],['--recoil-rescattering'],
                    ['--recoil-rescattering','on','--recoil-rescattering','off'],
                    ['--recoil-cuts','0'],['--recoil-cuts','nan','5'],['--recoil-cuts','0','-1'],
                    ['--recoil-cuts','0','5','--recoil-cuts','0','5']]:
        output = root/'invalid.jsonl'
        result = subprocess.run([runner,str(output),'1','2601001','medium',*options],capture_output=True,timeout=10)
        assert result.returncode and not output.exists()
print('Recoil controls route consistently; active failure is retained, not counted as parity')
