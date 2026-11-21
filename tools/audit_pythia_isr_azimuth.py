"""Audit ISR azimuth loss from recorded rotation factors (diagnostic only)."""
import argparse
import hashlib
import json
import math
from pathlib import Path

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('output',type=Path)
parser.add_argument('pairs',nargs='+',type=Path,help='alternating source trace and adapter snapshot')
args=parser.parse_args()
if len(args.pairs)%2:parser.error('expected trace/snapshot pairs')
u=2**-53
gamma7=7*u/(1-7*u)
reports=[]
for trace,snapshot in zip(args.pairs[::2],args.pairs[1::2]):
    branches={};events={}
    for line in trace.open():
        r=json.loads(line)
        if 'matrix' in r:
            key=(r['attempt'],r['emitted'])
            assert key not in branches
            branches[key]=r
        else:
            assert r['attempt'] not in events
            events[r['attempt']]=r['particles']
    expected=set();event_count=0
    for line in snapshot.open():
        e=json.loads(line)
        if e['type']!='event':continue
        assert events[e['attempt']]==[[p['pdg'],p['status'],*p['momentum']] for p in e['particles']], 'source instrumentation changed particles'
        event_count+=1
        for system,chains in enumerate(e['subsystem_isr_chains']):
            for chain in chains:
                for branch in chain['branches']:
                    key=(e['attempt'],branch['emitted'])
                    assert key not in expected
                    expected.add(key)
                    assert branches[key]['system']==system
                    assert branches[key]['mother']==branch['mother']
    assert event_count==len(events) and expected==set(branches)
    failures=[];maximum_error=0.;maximum_ratio=0.;worst=None
    for (attempt,emitted),r in branches.items():
        # Trace IDs are one-based; the particle list retains Pythia's slot zero.
        px,py=events[attempt][emitted-1][2:4]
        phi,theta=r['inputs'][8:10]
        b=r['rest_boost'];v=r['sister_rest_e_xyz'];m=r['from_rest_matrix']
        assert v[2]==0 and b[2]==b[6]==b[8]==b[9]==b[11]==b[14]==0
        pt=math.hypot(px,py)
        assert pt>0, 'azimuth undefined for zero transverse momentum'
        # R*B coefficient rounding and four-term matrix multiplication, followed
        # by a four-term matrix-vector dot. B and theta are common fixed inputs:
        # their errors cannot break the exact x/y proportionality by themselves.
        scale=abs(math.cos(theta))*sum(abs(b[4+i]*v[i]) for i in range(4))
        scale+=abs(math.sin(theta))*sum(abs(b[12+i]*v[i]) for i in range(4))
        errors=[(u+gamma7*(1+u))*abs(f(phi))*scale
                +gamma7*sum(abs(m[row*4+i]*v[i]) for i in range(4))
                for row,f in [(1,math.cos),(2,math.sin)]]
        bound=math.asin(min(1.,math.hypot(*errors)/pt))+8*u*math.pi
        error=abs(math.remainder(math.atan2(py,px)-phi,math.pi))
        ratio=error/bound
        maximum_error=max(maximum_error,error)
        if worst is None or ratio>maximum_ratio:
            maximum_ratio=ratio
            worst=dict(attempt=attempt,emitted=emitted,error_rad=error,bound_rad=bound)
        if error>bound:failures.append(dict(attempt=attempt,emitted=emitted,error_rad=error,bound_rad=bound))
    reports.append(dict(events_with_identical_particles=event_count,branches=len(branches),
        maximum_error_rad=maximum_error,maximum_error_to_bound=maximum_ratio,worst=worst,failures=failures,
        trace_sha256=hashlib.sha256(trace.read_bytes()).hexdigest(),
        snapshots_sha256=hashlib.sha256(snapshot.read_bytes()).hexdigest()))
report=dict(scope='azimuth reconstruction only; no acceptance or timing changes',
    assumptions=['IEEE binary64 round-to-nearest arithmetic without overflow or underflow',
                 'libm angle/trigonometric allowance 8*u*pi; not a certified platform-independent bound'],
    unit_roundoff=u,dot_gamma7=gamma7,reports=reports)
args.output.write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
if any(r['failures'] for r in reports):raise SystemExit('azimuth error exceeds diagnostic bound')
