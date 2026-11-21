"""Validate subsystem ISR matrices and their composition with primordial recoil."""
import collections
import hashlib
import itertools
import json
import math
from pathlib import Path
import sys

reports=[]
assert len(sys.argv)>=4 and len(sys.argv[2:])%2==0
for current,prior in zip(sys.argv[2::2],sys.argv[3::2]):
    counts=collections.Counter();failures=[];composition_failures=[]
    maximum_copy=maximum_composed=maximum_metric=0.
    with open(current) as a,open(prior) as b:
        for line,old_line in itertools.zip_longest(a,b):
            assert line is not None and old_line is not None
            e,old=json.loads(line),json.loads(old_line)
            if e['type']!='event':
                assert e==old
                continue
            frames=e.pop('subsystem_isr_frames');error=e.pop('subsystem_isr_frame_error')
            assert e==old, 'ISR frame mapping changed existing event data'
            counts['events']+=1
            if error:
                assert frames is None
                counts['rejected_events']+=1
                failures.append(dict(attempt=e['attempt'],reason=error))
                continue
            assert frames is not None and len(frames)==len(e['subsystems'])
            assert e['primordial_subsystem_error'] is None
            counts['mapped_events']+=1
            p={node['id']:node for node in e['particles']}
            def momentum(i):
                node=p[i]
                return [node['energy_before_beam_repair'],*node['momentum'][:3]]
            def matrix_apply(m,v):
                return [math.fsum(x*y for x,y in zip(m[k:k+4],v)) for k in range(0,16,4)]
            all_copies=set();all_emitted=set()
            for system,fs,primordial in zip(e['subsystems'],frames,e['primordial_subsystem_frames']):
                assert [f['emitted'] for f in fs]==sorted(f['emitted'] for f in fs)
                links={}
                for f in fs:
                    counts['frames']+=1
                    assert f['momentum_stage']=='before_beam_energy_repair'
                    assert f['emitted'] not in all_emitted;all_emitted.add(f['emitted'])
                    m=f['matrix_e_xyz'];assert len(m)==16 and all(math.isfinite(v) for v in m)
                    norm=max(sum(abs(v) for v in m[k:k+4]) for k in range(0,16,4))
                    for i in range(4):
                        for j in range(4):
                            residual=abs(math.fsum((1 if k==0 else -1)*m[4*k+i]*m[4*k+j] for k in range(4))-((1 if i==0 else -1) if i==j else 0))
                            maximum_metric=max(maximum_metric,residual)
                            assert residual<=1e-7*max(1,norm*norm)
                    for before,after in f['copies']:
                        counts['copies']+=1
                        assert after not in all_copies;all_copies.add(after)
                        assert abs(p[after]['status'])==44 and p[after]['mothers']==[before]
                        assert before not in links;links[before]=(after,m)
                        v=momentum(before);actual=momentum(after);predicted=matrix_apply(m,v)
                        for k,(x,y) in enumerate(zip(predicted,actual)):
                            residual=abs(x-y);maximum_copy=max(maximum_copy,residual)
                            dot_roundoff=16*math.ulp(max(1.,sum(abs(x*y) for x,y in zip(m[4*k:4*k+4],v))))
                            assert residual<=2e-9+2e-11*abs(y)+dot_roundoff
                for before,after in primordial['copies']:
                    assert before not in links
                    links[before]=(after,primordial['matrix_e_xyz'])
                roots=system['process_outgoing']+[f['emitted'] for f in fs]
                finals=set()
                for root in roots:
                    v=momentum(root);i=root;visited=set()
                    while not p[i]['final']:
                        assert i not in visited and i in links
                        visited.add(i);i,m=links[i];v=matrix_apply(m,v)
                    assert i not in finals;finals.add(i);counts['composed_final_partons']+=1
                    residuals=[];composed_ok=True
                    for x,y in zip(v,momentum(i)):
                        residual=abs(x-y);maximum_composed=max(maximum_composed,residual)
                        residuals.append(residual);composed_ok=composed_ok and residual<=1e-8+1e-10*abs(y)
                    if not composed_ok:composition_failures.append(dict(attempt=e['attempt'],subsystem=system['index'],root=root,final=i,max_absolute_error_gev=max(residuals)))
                assert finals==set(system['outgoing_final'])
            assert all_emitted==set(e['isr_emissions'])
            assert all_copies=={i for i,node in p.items() if abs(node['status'])==44}
    reports.append(dict(path=current,sha256=hashlib.sha256(Path(current).read_bytes()).hexdigest(),
        prior_sha256=hashlib.sha256(Path(prior).read_bytes()).hexdigest(),counts=dict(counts),
        rejected_events=failures,composition_failures=composition_failures,max_copy_residual_gev=maximum_copy,
        max_composed_residual_gev=maximum_composed,max_metric_residual=maximum_metric))
Path(sys.argv[1]).write_text(json.dumps(dict(scope='partial subsystem ISR reconstruction and staged primordial composition',reports=reports),indent=2)+'\n')
print(json.dumps([{k:v for k,v in r.items() if k not in ('rejected_events','composition_failures')} for r in reports],indent=2))

if any(r["composition_failures"] for r in reports):
    raise SystemExit("composed transport has failed comparisons; see recorded evidence")
