"""Audit reconstructed MPI primordial frames without treating rejections as passes."""
import collections
import hashlib
import json
import math
from pathlib import Path
import sys

reports=[]
for current,prior in zip(sys.argv[2::2],sys.argv[3::2]):
    counts=collections.Counter(); reasons=collections.Counter()
    max_copy=max_metric=max_roundoff=0.
    with open(current) as a,open(prior) as b:
        for line,old_line in __import__('itertools').zip_longest(a,b):
            assert line is not None and old_line is not None
            event,old=json.loads(line),json.loads(old_line)
            if event['type']!='event':
                assert event==old
                continue
            frames=event.pop('primordial_subsystem_frames');error=event.pop('primordial_subsystem_error')
            assert event==old, 'new mapping changed existing event content'
            counts['events']+=1
            if error is not None:
                assert frames is None
                counts['rejected_events']+=1
                reasons[error.split(':')[0]]+=1
                continue
            counts['mapped_events']+=1
            assert len(frames)==len(event['subsystems'])
            p={node['id']:node for node in event['particles']};copies=set()
            for system,f in zip(event['subsystems'],frames):
                counts['frames']+=1
                assert f['incoming_after']==system['incoming']
                assert {after for before,after in f['copies']}==set(system['incoming']+system['outgoing'])
                m=[f['matrix_e_xyz'][i:i+4] for i in range(0,16,4)]
                assert all(math.isfinite(x) for row in m for x in row)
                norm=max(sum(abs(x) for x in row) for row in m)
                for i in range(4):
                    for j in range(4):
                        metric=math.fsum((1 if k==0 else -1)*m[k][i]*m[k][j] for k in range(4))
                        error_metric=abs(metric-((1 if i==0 else -1) if i==j else 0))
                        max_metric=max(max_metric,error_metric)
                        assert error_metric<=1e-7*max(1,norm*norm)
                allowance=f['maximum_roundoff_allowance_gev'];max_roundoff=max(max_roundoff,allowance)
                for before,after in f['copies']:
                    counts['copies']+=1
                    assert after not in copies;copies.add(after)
                    x=p[before]['momentum'];v=[x[3],*x[:3]]
                    x=p[after]['momentum'];actual=[x[3],*x[:3]]
                    for row,wanted in zip(m,actual):
                        predicted=math.fsum(x*y for x,y in zip(row,v));residual=abs(predicted-wanted)
                        max_copy=max(max_copy,residual)
                        # fsum differs from the adapter's sequential double sum.
                        dot_roundoff=16*math.ulp(max(1.,sum(abs(x*y) for x,y in zip(row,v))))
                        assert residual<=2e-9+2e-11*abs(wanted)+allowance+dot_roundoff
            assert copies=={i for i,node in p.items() if abs(node['status']) in (61,62)}
    reports.append(dict(path=current,sha256=hashlib.sha256(Path(current).read_bytes()).hexdigest(),
        prior_sha256=hashlib.sha256(Path(prior).read_bytes()).hexdigest(),counts=dict(counts),
        rejection_reasons=dict(reasons),maximum_copy_residual_gev=max_copy,
        maximum_metric_residual=max_metric,maximum_roundoff_allowance_gev=max_roundoff))
Path(sys.argv[1]).write_text(json.dumps(dict(scope='partial MPI primordial reconstruction; failures retained',reports=reports),indent=2)+'\n')
print(json.dumps(reports,indent=2))
