"""Hard cuts must reach Pythia and the JEWEL table-energy prescription."""
import hashlib
import json
import math
from pathlib import Path
import subprocess
import sys
import tempfile

adapter, runner, output = map(Path,sys.argv[1:])
adapter,runner=adapter.resolve(),runner.resolve()
report={'cases':[], 'invalid_inputs_rejected':0}
with tempfile.TemporaryDirectory() as tmp:
    root=Path(tmp)
    def run(exe,label,options,count=1,replay=False):
        path=root/(label+'.jsonl'); command=[str(exe),str(path),str(count),'2601001']
        if exe==runner:command+=['vacuum']
        if replay:command+=[str(root/label)]
        result=subprocess.run(command+options,capture_output=True,text=True,timeout=90)
        if result.returncode:raise RuntimeError(result.stderr[-3000:])
        return [json.loads(line) for line in path.read_text().splitlines()]
    default=run(adapter,'default',[],10)
    explicit=run(adapter,'explicit',['--ecm','5020','--pt-min','20','--pt-max','100'],10)
    assert default==explicit,'explicit defaults changed generator records'
    assert run(runner,'runner-default',[]) == run(runner,'runner-explicit',
        ['--ecm','5020','--pt-min','20','--pt-max','100']), 'runner defaults changed'
    for ecm,low,high in [(2760,50,400),(13000,40,60)]:
        options=['--ecm',str(ecm),'--pt-min',str(low),'--pt-max',str(high)]
        rows=run(adapter,f'adapter-{ecm}',options,100)
        settings=dict(s.split(' = ',1) for s in rows[0]['settings'])
        for key,value in [('Beams:eCM',ecm),('PhaseSpace:pTHatMin',low),('PhaseSpace:pTHatMax',high)]:
            assert float(settings[key])==value
        events=[r for r in rows if r['type']=='event']
        assert len(events)==100
        assert all(low<=r['pt_hat']<=high for r in events)
        label=f'runner-{ecm}'
        rows=run(runner,label,options+['--table-energy','jewel'],replay=True)
        expected=min(1.5*(high+50)*math.cosh(2),ecm/2)
        assert rows[0]['table_energy_gev']==expected
        assert rows[0]['table_energy_prescription']=='jewel-eovest'
        settings=dict(s.split(' = ',1) for s in rows[0]['pythia_settings'])
        assert float(settings['Beams:eCM'])==ecm
        assert float(settings['PhaseSpace:pTHatMin'])==low and float(settings['PhaseSpace:pTHatMax'])==high
        fixture=(root/label/'1.input').read_text().splitlines()
        for tag in ['FCONFIG','RCONFIG','PCONFIG']:
            assert float(fixture[fixture.index(tag)+1].split()[3])==expected
        assert float(fixture[fixture.index('XCONFIG')+1].split()[4])==expected
        report['cases'].append(dict(ecm=ecm,pt_min=low,pt_max=high,hard_events=len(events),
            table_energy_gev=expected,native_outcome=rows[1]['type']))
    invalid=[]
    for flag in ['--ecm','--pt-min','--pt-max']:
        invalid.extend([[flag],[flag,'nan'],[flag,'inf'],[flag,'0'],[flag,'-1'],[flag,'20x'],[flag,'50',flag,'60']])
    invalid.extend([['--pt-min','100'],['--pt-max','10'],['--ecm','30']])
    for exe in [adapter,runner]:
        for options in invalid:
            path=root/'invalid.jsonl';command=[str(exe),str(path),'1','2601001']
            if exe==runner:command+=['vacuum']
            result=subprocess.run(command+options,capture_output=True,timeout=15)
            assert result.returncode!=0 and not path.exists(),options
            report['invalid_inputs_rejected']+=1
report.update(default_records_identical=True,executables={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in [adapter,runner]})
output.write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
