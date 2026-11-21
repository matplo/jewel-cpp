"""Check ISR matrices, copy closure and displacement round trips against prior snapshots."""
import json,math,hashlib,sys
from pathlib import Path
reports=[]
for path,prior in [(Path(sys.argv[1]),Path(sys.argv[2]))]:
 rows=[json.loads(x) for x in path.read_text().splitlines()];old=[json.loads(x) for x in prior.read_text().splitlines()]
 seed=rows[0]['seed'];assert len(rows)==len(old)
 nframes=ncopies=0;maxmetric=maxcopy=maxroundtrip=0
 for e,original in zip(rows,old):
  if e['type']!='event':assert e==original;continue
  assert {k:v for k,v in e.items() if k!='isr_recoil_frames'}==original
  assert [f['emitted'] for f in e['isr_recoil_frames']]==sorted(e['isr_emissions'])
  p={x['id']:x for x in e['particles']}
  for f in e['isr_recoil_frames']:
   nframes+=1;m=[f['matrix_e_xyz'][4*i:4*i+4] for i in range(4)];g=[1,-1,-1,-1]
   error=max(abs(sum(g[k]*m[k][i]*m[k][j] for k in range(4))-(g[i] if i==j else 0)) for i in range(4) for j in range(4));maxmetric=max(maxmetric,error);assert error<1e-9
   inverse=[[g[i]*g[j]*m[j][i] for j in range(4)] for i in range(4)]
   displacement=[1.,.2,-.4,.1]
   transformed=[math.fsum(a*b for a,b in zip(row,displacement)) for row in m]
   back=[math.fsum(a*b for a,b in zip(row,transformed)) for row in inverse]
   error=max(abs(a-b) for a,b in zip(displacement,back));maxroundtrip=max(maxroundtrip,error);assert error<1e-9
   localmax=0;arithmetic_bound=0
   for before,after in f['copies']:
    ncopies+=1;v=p[before]['momentum'];target=p[after]['momentum'];v=[v[3],*v[:3]];target=[target[3],*target[:3]]
    arithmetic_bound=max(arithmetic_bound,32*math.ulp(max(1.,max(sum(abs(a*b) for a,b in zip(row,v)) for row in m))))
    transformed=[sum(a*b for a,b in zip(row,v)) for row in m]
    for a,b in zip(transformed,target):
     err=abs(a-b);maxcopy=max(maxcopy,err);localmax=max(localmax,err);assert err<=2e-9+2e-11*abs(b)
   assert abs(localmax-f['maximum_copy_error_gev'])<=arithmetic_bound,(seed,e['attempt'],f['emitted'],localmax,f['maximum_copy_error_gev'],arithmetic_bound,m)
 reports.append(dict(seed=seed,events=sum(e['type']=='event' for e in rows),frames=nframes,momentum_copies=ncopies,max_copy_error_gev=maxcopy,
  max_lorentz_metric_error=maxmetric,max_displacement_roundtrip_error_fm=maxroundtrip,prior_fields_exactly_unchanged=True,
  sha256=hashlib.sha256(path.read_bytes()).hexdigest(),input=str(path)))
print(json.dumps(reports,indent=2));Path(sys.argv[3]).write_text(json.dumps(reports,indent=2)+'\n')
