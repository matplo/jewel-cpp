"""Diagnose signed spacelike energies after common-final-frame transport.

Counts and invariant residuals are diagnostics, not a timing prescription or
physics acceptance gate. This does not modify recorded momentum or virtuality.
"""
import argparse,hashlib,json,math
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('inputs',type=Path,nargs='+');p.add_argument('--output',type=Path,required=True)
a=p.parse_args();reports=[]
for path in a.inputs:
    n=negative_before=negative_after=changed=0;max_error=max_relative=0
    for line in path.open():
        e=json.loads(line)
        if e['type']!='event':continue
        frames=e['isr_recoil_frames'];last=e['primordial_recoil_frame']
        assert [f['emitted'] for f in frames]==sorted(e['isr_emissions'])
        for chain in e['isr_chains']:
            for branch in chain['branches']:
                q=branch['spacelike_daughter'];v=[q[3],*q[:3]];before=v[0]<0
                for frame in [*[f for f in frames if f['emitted']>branch['emitted']],last]:
                    m=frame['matrix_e_xyz']
                    v=[math.fsum(m[4*i+j]*v[j] for j in range(4)) for i in range(4)]
                if not all(math.isfinite(x) for x in v):raise ValueError('nonfinite transported branch')
                n+=1;negative_before+=before;negative_after+=v[0]<0;changed+=before!=(v[0]<0)
                error=abs(math.fsum(x*x for x in v[1:])-v[0]*v[0]-branch['virtuality_squared'])
                max_error=max(max_error,error);max_relative=max(max_relative,error/max(1.,branch['virtuality_squared']))
    reports.append(dict(input=str(path),input_sha256=hashlib.sha256(path.read_bytes()).hexdigest(),branches=n,
        negative_stage_energy=negative_before,negative_final_frame_energy=negative_after,sign_changes=changed,
        max_invariant_absolute_residual_gev2=max_error,max_invariant_residual_over_max_1_q2=max_relative))
report=dict(scope=__doc__.strip(),samples=reports)
a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
