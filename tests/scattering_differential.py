import array,hashlib,json,math,subprocess,sys,tempfile,time
from pathlib import Path
cpp,reference,output=sys.argv[1:]
groups={};failures=[];records=[];table_preservation=[]
def compare(name,a,b):
 assert len(a)==len(b)
 g=groups.setdefault(name,dict(values=0,max_absolute_error=0.,max_relative_error=0.,outside_tolerance=0))
 for i,(x,y) in enumerate(zip(a,b)):
  error=abs(x-y);g['values']+=1;g['max_absolute_error']=max(g['max_absolute_error'],error);g['max_relative_error']=max(g['max_relative_error'],error/max(abs(y),1e-300))
  if not math.isfinite(x) or not math.isfinite(y) or (x==0)!=(y==0) or not math.isclose(x,y,rel_tol=2e-12,abs_tol=2e-13):
   g['outside_tolerance']+=1
   if len(failures)<30:failures.append(dict(group=name,index=i,cpp=x,fortran=y))
# Preserve the original cases and add independent medium/table combinations.
# The screening scale is distinct from the scattering renormalization scale.
configs=[
 (3,1,.2,.2,20,1,0,.45,.4,.59,.17,1,1),
 (5,1.5,.25,.18,50,1.2,1,.45,.4,.59,.17,1,1),
 (3,1,.2,.2,20,1,0,2.,.4,.59,.17,1,1),
 (3,1,.2,.2,100,1,0,.45,.6,.45,.15,.6,0),
 (3,1,.2,.2,1.5*150*math.cosh(2),1,1,.45,.3,.7,.12,1.2,1),
 (5,1.5,.25,.18,2510,1.2,1,.9,.6,.45,.15,1.2,0),
]
with tempfile.TemporaryDirectory() as tmp:
 for nf,q0,lps,lqcd,emax,scale,boost,floor,taui,ti,tc,screening,woods in configs:
  medium_config=dict(tau_initial_fm=taui,temperature_initial_gev=ti,
   temperature_cutoff_gev=tc,screening_scale=screening,woods_saxon=bool(woods))
  base=f'CONFIG\n208 {nf} {woods} {taui} {ti} {tc} .17 .54 6.199999809265137 3 2 {floor} {screening}\nXCONFIG\n{nf} {q0} {lps} {lqcd} {emax:.17g} {scale} {boost}\n'
  queries=[]
  for c in ['QA','QB','GA','GB']:
   for t in [.5,1,2,10,100,1000]:
    for md in [.45,.51,.8,1.5,2]:queries.append(f'XINT\n{c} {t} {md}\n')
  for incoming in ['Q','G']:
   for outgoing in ['Q','G','C']:
    for mode in [0,1,2]:
     for momentum in [2,10,30]:
      for z,t in [(0,1),(math.sinh(1),math.cosh(1)),(-math.sinh(1),math.cosh(1))]:
       mass=.3;energy=math.sqrt(momentum*momentum+mass*mass)
       queries.append(f'XSEC\n{momentum} 0 0 {energy:.17g} {mass} 0 {incoming} {outgoing} 0 0 {z:.17g} {t:.17g} {mode}\n')
  # Exercise rates before/at initialization and on the cooling branch,
  # at transverse points inside and outside the medium, with both flow signs.
  for tau in [taui/2,taui,2*taui,10,100]:
   for x,y in [(0,0),(3,1),(20,0)]:
    for eta in [-1.5,0,1.5]:
     z,t=tau*math.sinh(eta),tau*math.cosh(eta)
     for incoming in ['Q','G']:
      for mode in [0,1,2]:
       queries.append(f'XSEC\n6 0 8 10 0 0 {incoming} C {x} {y} {z:.17g} {t:.17g} {mode}\n')
  queries.append('XSEC\n10 0 0 10 0 1000 Q C 0 0 0 1 0\n')
  queries.append('XSEC\n10 0 0 10 0 0 Q C 0 0 0 0 0\n')
  # Prescribed massless trajectories couple local medium fields to the
  # independently initialized rate tables. These are not evolved cascades.
  ray_queries=[]
  ray_offsets=[0.,.05,.1,.15,.25,.35,.55,.75]+[float(i) for i in range(1,31)]
  for x0,y0 in [(0,0),(2,1),(8,0)]:
   for eta in [-1.5,0,1.5]:
    for phi in [0,math.pi/2,math.pi,3*math.pi/2]:
     vx,vy,vz=math.cos(phi)/math.cosh(eta),math.sin(phi)/math.cosh(eta),math.tanh(eta)
     energy=10.
     for dt in ray_offsets:
      t=.05+dt
      x,y,z=x0+vx*dt,y0+vy*dt,vz*dt
      for incoming in ['Q','G']:
       for mode in [0,1,2]:
        ray_queries.append(f'XSEC\n{energy*vx:.17g} {energy*vy:.17g} {energy*vz:.17g} {energy} 0 0 {incoming} C {x:.17g} {y:.17g} {z:.17g} {t:.17g} {mode}\n')
  values=[];data=[]
  for exe in [cpp,reference]:
   dump=Path(tmp)/(Path(exe).name+'.bin');after=Path(tmp)/(Path(exe).name+'-after.bin')
   payload=base+'XDUMP\n'+str(dump)+'\n'+''.join(queries+ray_queries)+'XDUMP\n'+str(after)+'\n'
   start=time.monotonic();r=subprocess.run([exe],input=payload,text=True,capture_output=True,timeout=180)
   if r.returncode:raise RuntimeError((exe,r.returncode,r.stderr[-1500:]))
   raw=dump.read_bytes();a=array.array('d');a.frombytes(raw);assert len(a)==401100
   data.append(a);values.append([float(line) for line in r.stdout.splitlines()])
   assert len(values[-1])==len(queries)+len(ray_queries)
   unchanged=raw==after.read_bytes()
   table_preservation.append(dict(executable=Path(exe).name,configuration_index=len(table_preservation)//2,unchanged=unchanged))
   if not unchanged:failures.append(dict(group='table-preservation',executable=Path(exe).name))
   records.append(dict(executable=Path(exe).name,nf=nf,q0=q0,lambda_shower=lps,lambda_scattering=lqcd,emax=emax,scale=scale,boost=boost,debye_floor=floor,medium=medium_config,query_sha256=hashlib.sha256((base+''.join(queries+ray_queries)).encode()).hexdigest(),seconds=time.monotonic()-start,table_sha256=hashlib.sha256(raw).hexdigest()))
   print('built',Path(exe).name,'NF',nf,flush=True)
  compare('full-grids',data[0],data[1]);compare('integrals-and-cross-sections',values[0][:len(queries)],values[1][:len(queries)])
  ray_values=[v[len(queries):] for v in values]
  compare('expanding-ray-cross-sections',*ray_values)
  records.append(dict(scope='expanding-ray-cross-sections',medium=medium_config,
   table_energy_gev=emax,boost=boost,rays=36,samples_per_ray=len(ray_offsets),
   lab_times_fm=[.05+dt for dt in ray_offsets],
   by_mode={str(mode):dict(queries=len(ray_values[1][mode::3]),
    reference_zero_values=sum(v==0 for v in ray_values[1][mode::3]),
    reference_nonzero_values=sum(v!=0 for v in ray_values[1][mode::3])) for mode in range(3)},
   queries=len(ray_queries),reference_zero_values=sum(v==0 for v in ray_values[1]),
   reference_nonzero_values=sum(v!=0 for v in ray_values[1]),
   input_sha256=hashlib.sha256((base+''.join(ray_queries)).encode()).hexdigest()))
  # Use reference coordinates for identical queries in both independently built
  # engines. Probe every interpolation-cell boundary and adjacent doubles.
  transfers=list(data[1][:1000]);debye=list(data[1][1000:1100])
  neighbours=lambda x: (math.nextafter(x,-math.inf),x,math.nextafter(x,math.inf))
  boundary_queries=[]
  for channel in ['QA','QB','GA','GB']:
   for t in transfers:
    for near in neighbours(t):
     for md in sorted(set([debye[0],debye[49],debye[-1]])):
      boundary_queries.append(f'XINT\n{channel} {near:.17g} {md:.17g}\n')
   for md in sorted(set(debye)):
    for near in neighbours(md):
     for t in [transfers[0],transfers[499],transfers[-1]]:
      boundary_queries.append(f'XINT\n{channel} {t:.17g} {near:.17g}\n')
   # Stock linear extrapolation outside both axes is part of the contract.
   for t in [transfers[0]/2,transfers[-1]*2]:
    for md in [debye[0]/2,debye[-1]*2]:
     boundary_queries.append(f'XINT\n{channel} {t:.17g} {md:.17g}\n')
  boundary_payload=base+''.join(boundary_queries)
  boundary_results=[]
  for exe in [cpp,reference]:
   r=subprocess.run([exe],input=boundary_payload,text=True,capture_output=True,timeout=180)
   if r.returncode:raise RuntimeError((exe,r.returncode,r.stderr[-1500:]))
   boundary_results.append([float(line) for line in r.stdout.splitlines()])
  assert all(len(result)==len(boundary_queries) for result in boundary_results)
  compare('grid-boundaries-and-extrapolation',*boundary_results)
  records.append(dict(scope='grid-boundaries-and-extrapolation',nf=nf,boost=boost,
   debye_floor=floor,medium=medium_config,table_energy_gev=emax,queries=len(boundary_queries),
   input_sha256=hashlib.sha256(boundary_payload.encode()).hexdigest()))
report=dict(scope='scattering grids and channel cross sections, not transport events',groups=groups,failures=failures,records=records,
 table_preservation=table_preservation,
 zero_nonzero_agreement_required=True,relative_tolerance=2e-12,absolute_tolerance=2e-13,
 sources=json.loads((Path(__file__).parents[1]/'reference/medium-source.json').read_text()),
 executables={Path(exe).name:hashlib.sha256(Path(exe).read_bytes()).hexdigest() for exe in [cpp,reference]})
Path(output).write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(groups,indent=2))
if failures:print(failures[:10]);raise SystemExit('Scattering parity failed')
print('PASS',sum(g['values'] for g in groups.values()),'comparisons')
