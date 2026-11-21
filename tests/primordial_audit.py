"""Independent serialized primordial-matrix checks through original copy records."""
import argparse,hashlib,json,math
from pathlib import Path
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('inputs',type=Path,nargs='+');parser.add_argument('--output',type=Path,required=True)
args=parser.parse_args();reports=[]
for path in args.inputs:
    events=0;copies=0;transported=0;maxtransport=0;maxcopy=maxmetric=maxroundtrip=0
    for line in path.open():
        e=json.loads(line)
        if e['type']!='event':continue
        events+=1;frame=e['primordial_recoil_frame'];p={x['id']:x for x in e['particles']}
        assert len(frame['incoming_before'])==len(frame['incoming_after'])==2
        assert set(frame['incoming_after'])=={x['id'] for x in e['particles'] if abs(x['status'])==61}
        expected={x['id'] for x in e['particles'] if abs(x['status']) in [61,62]}
        assert len(frame['copies'])==len(expected) and {pair[1] for pair in frame['copies']}==expected
        m=[frame['matrix_e_xyz'][4*i:4*i+4] for i in range(4)];g=[1,-1,-1,-1]
        error=max(abs(math.fsum(g[k]*m[k][i]*m[k][j] for k in range(4))-(g[i] if i==j else 0)) for i in range(4) for j in range(4))
        maxmetric=max(maxmetric,error);assert error<1e-9
        inverse=[[g[i]*g[j]*m[j][i] for j in range(4)] for i in range(4)]
        for delta in [[1.,.2,-.4,.1],[-.5,1.,2.,-3.],[0.,0.,0.,0.]]:
            transformed=[math.fsum(a*b for a,b in zip(row,delta)) for row in m]
            back=[math.fsum(a*b for a,b in zip(row,transformed)) for row in inverse]
            error=max(abs(a-b) for a,b in zip(delta,back));maxroundtrip=max(maxroundtrip,error);assert error<1e-9
        localmax=bound=0
        for before,after in frame['copies']:
            copies+=1;v=p[before]['momentum'];target=p[after]['momentum'];v=[v[3],*v[:3]];target=[target[3],*target[:3]]
            bound=max(bound,32*math.ulp(max(1.,max(sum(abs(a*b) for a,b in zip(row,v)) for row in m))))
            actual=[sum(a*b for a,b in zip(row,v)) for row in m]
            for a,b in zip(actual,target):
                error=abs(a-b);maxcopy=max(maxcopy,error);localmax=max(localmax,error);assert error<=2e-9+2e-11*abs(b)
        assert abs(localmax-frame['maximum_copy_error_gev'])<=bound
        # Follow every hard/emitted root through all ISR recoil stages and the
        # primordial stage. No newly assigned physical production times enter.
        transitions={}
        for stage in [*e.get('isr_recoil_frames',[]),frame]:
            matrix=[stage['matrix_e_xyz'][4*i:4*i+4] for i in range(4)]
            for before,after in stage['copies']:
                assert before not in transitions
                transitions[before]=(after,matrix)
        roots=[*e['hard_outgoing'],*e['isr_emissions']]
        targets=[*e['hard_final'],*e['isr_final']]
        for root,target in zip(roots,targets):
            v=p[root]['momentum'];v=[v[3],*v[:3]];visited=set();current=root
            while current!=target:
                assert current not in visited;visited.add(current)
                current,matrix=transitions[current]
                v=[math.fsum(a*b for a,b in zip(row,v)) for row in matrix]
            expected=p[target]['momentum'];expected=[expected[3],*expected[:3]]
            for a,b in zip(v,expected):
                error=abs(a-b);maxtransport=max(maxtransport,error)
                assert error<=1e-8+1e-10*abs(b)
            transported+=1
    assert events>0
    reports.append(dict(input=str(path),sha256=hashlib.sha256(path.read_bytes()).hexdigest(),events=events,copies=copies,
        transported_final_partons=transported,max_composed_momentum_error_gev=maxtransport,
        max_copy_error_gev=maxcopy,max_lorentz_metric_error=maxmetric,max_displacement_roundtrip_error_fm=maxroundtrip))
args.output.write_text(json.dumps(reports,indent=2)+'\n');print(json.dumps(reports,indent=2))
