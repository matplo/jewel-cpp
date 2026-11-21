"""Verify diagnostic events and replay actual source ISR matrices through the audit."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('trace',type=Path);p.add_argument('snapshots',type=Path);p.add_argument('output',type=Path)
a=p.parse_args();traces={};observed={}
for line in a.trace.open():
    r=json.loads(line)
    if 'matrix' in r:
        key=(r['attempt'],r['emitted']);assert key not in traces,'ambiguous trace history';traces[key]=r
    else:
        assert r['attempt'] not in observed;observed[r['attempt']]=r['particles']
records=[];used=set();events=0
parameter_errors={key:dict(max_absolute_error=0.,max_relative_error=0.) for key in ['Q2','z','m2II']}
for line in a.snapshots.open():
    e=json.loads(line);records.append(e)
    if e['type']!='event':continue
    events+=1
    assert observed.pop(e['attempt'])==[[p['pdg'],p['status'],*p['momentum']] for p in e['particles']], 'instrumented source changed generated particles'
    particles={p['id']:p for p in e['particles']}
    e['subsystem_isr_frames']=[];e['subsystem_isr_frame_error']=None
    for index,chains in enumerate(e['subsystem_isr_chains']):
        frames=[]
        for chain in chains:
            for branch in chain['branches']:
                key=(e['attempt'],branch['emitted']);assert key not in used;used.add(key)
                r=traces[key];assert r['system']==index and r['mother']==branch['mother']
                energy=lambda i:particles[i]['energy_before_beam_repair']
                old_recoiler=particles[r['recoiler']]['daughters'][0]
                reconstructed=[branch['virtuality_squared'],energy(branch['daughter'])/energy(branch['mother']),4*energy(branch['daughter'])*energy(old_recoiler)]
                for name,actual,expected in zip(parameter_errors,reconstructed,r['inputs'][:3]):
                    error=abs(actual-expected);stats=parameter_errors[name]
                    stats['max_absolute_error']=max(stats['max_absolute_error'],error)
                    stats['max_relative_error']=max(stats['max_relative_error'],error/max(abs(expected),1e-300))
                copies=[[particles[i]['mothers'][0],i] for i in range(max(r['mother'],r['recoiler'])+1,r['emitted'])]
                frames.append(dict(momentum_stage='before_beam_energy_repair',emitted=r['emitted'],mother=r['mother'],
                    recoiler=r['recoiler'],old_recoiler=particles[r['recoiler']]['daughters'][0],matrix_e_xyz=r['matrix'],copies=copies))
        frames.sort(key=lambda f:f['emitted']);e['subsystem_isr_frames'].append(frames)
assert not observed and used==set(traces)
with tempfile.TemporaryDirectory() as directory:
    replay=Path(directory)/'replay.jsonl';result=Path(directory)/'audit.json'
    replay.write_text(''.join(json.dumps(e)+'\n' for e in records))
    audit=Path(__file__).resolve().parents[1]/'tests/mpi_isr_frames_audit.py'
    run=subprocess.run([sys.executable,str(audit),str(result),str(replay),str(a.snapshots)],capture_output=True,text=True)
    if not result.exists():raise RuntimeError(run.stdout+run.stderr)
    report=json.loads(result.read_text());detail=report['reports'][0]
    detail.pop('path');detail.pop('sha256') # Temporary replay path is not a retained artifact.
report.update(scope='actual Pythia source matrices; composition uses validated primordial reconstruction',
 reconstructed_parameter_errors=parameter_errors,
 events_with_identical_particles=events,accepted_source_branches=len(used),composition_audit_returncode=run.returncode,
 trace_sha256=hashlib.sha256(a.trace.read_bytes()).hexdigest(),snapshots_sha256=hashlib.sha256(a.snapshots.read_bytes()).hexdigest())
a.output.write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(dict(events=events,source_branches=len(used),composition_failures=len(detail['composition_failures']),max_error_gev=detail['max_composed_residual_gev'])))
if run.returncode:raise SystemExit('source composition still fails fixed tolerance; evidence recorded')
