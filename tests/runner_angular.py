"""All ANGORD values must route to the same branching mode in native replay."""
from concurrent.futures import ThreadPoolExecutor
import json
from pathlib import Path
import subprocess
import sys
import tempfile

runner, probe = [str(Path(p).resolve()) for p in sys.argv[1:]]
with tempfile.TemporaryDirectory() as temporary:
    root = Path(temporary)
    def run(mode):
        output, replay = root/f'{mode}.jsonl', root/f'{mode}-replay'
        subprocess.run([runner,str(output),'1','2601001','medium',str(replay),
                        '--centrality','0','10','--table-energy','jewel','--longitudinal-boost','on',
                        '--angular-ordering',str(mode)],check=True,capture_output=True,timeout=90)
        rows = [json.loads(line) for line in output.read_text().splitlines()]
        assert rows[0]['angular_ordering']==mode and rows[0]['first_angle_ordering'] is False
        event = rows[1];assert event['type']=='event',event
        fixture = (replay/'1.input').read_text();lines=fixture.splitlines()
        assert int(lines[lines.index('BCONFIG')+1].split()[-1])==mode
        result = subprocess.run([probe],input=fixture,text=True,capture_output=True,check=True,timeout=90)
        records = [list(map(float,line.split())) for line in result.stdout.splitlines()]
        assert records[0][0]==len(event['particles']) and records[0][4]==event['draws']
        assert records[0][5:7]==[0,0]
        for particle,record in zip(event['particles'],records[1:]):
            assert particle['momentum']==record[5:9] and particle['status']==record[0]
        return rows
    with ThreadPoolExecutor(max_workers=2) as pool:
        samples=list(pool.map(run,range(6)))
    output=root/'default.jsonl'
    subprocess.run([runner,str(output),'1','2601001','medium','--centrality','0','10',
                    '--table-energy','jewel','--longitudinal-boost','on'],check=True,capture_output=True,timeout=90)
    assert [json.loads(line) for line in output.read_text().splitlines()]==samples[0]
    assert samples[3][1]['draws']!=samples[0][1]['draws']
    for values in [[],['-1'],['6'],['1.0'],['01'],['on'],['1','--angular-ordering','2']]:
        output=root/'invalid.jsonl'
        result=subprocess.run([runner,str(output),'1','2601001','medium','--angular-ordering',*values],capture_output=True,timeout=10)
        assert result.returncode and not output.exists()
print('Six angular modes reproduce native replay; default and argument checks pass')
