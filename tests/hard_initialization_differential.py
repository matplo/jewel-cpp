"""PPJJ FINDVIRTUALITIES replay against unchanged JEWEL 2.6.0."""
import hashlib,json,math,subprocess,sys
from pathlib import Path
cpp,reference,output=sys.argv[1:4]
mpi_mode=len(sys.argv)>4 and sys.argv[4]=="--mpi"
def row(values):return ' '.join(format(v,'.17g') for v in values)+'\n'
def boost(p,beta):
 x,y,z,e=p;bx,by,bz=beta;bs=bx*bx+by*by+bz*bz;gamma=1/math.sqrt(1-bs)
 dot=bx*x+by*y+bz*z;factor=gamma*(gamma*dot/(1+gamma)+e)
 return [x+factor*bx,y+factor*by,z+factor*bz,gamma*(e+dot)]
def record(status,pdg,parent,daughters,p,mass,colour,anti):
 return [status,pdg,parent,*daughters,*p,mass,.1,-.2,.3,.4,9,8,7,6,5,.4,.3,0,1,colour,anti,-1e6]
commands=[];settings=[]
for pdg1,pdg2 in [(21,21),(1,-1),(1,21),(-3,21)]:
 for rapidity in [0,.8,2.5]:
  for maximum in [.1,1,5,20,100,500]:
   for angular in [0,1]:
    for frozen in [0,1]:
     for mpi_fsr in ([0,1] if mpi_mode else [None]):
      for channel_swap in ([0,1] if mpi_mode else [0]):
       mass1=4.8 if pdg1!=21 and maximum>=100 else 0;mass2=0;momentum=30
       pz=momentum*math.sinh(rapidity) if mpi_mode else 0
       p1=[momentum*.6,momentum*.8,pz,math.sqrt(momentum**2+pz**2+mass1**2)]
       p2=[-p1[0],-p1[1],-pz,math.sqrt(momentum**2+pz**2+mass2**2)]
       p1=boost(boost(p1,[.2,0,0]),[0,0,math.tanh(rapidity)])
       p2=boost(boost(p2,[.2,0,0]),[0,0,math.tanh(rapidity)])
       total=[a+b for a,b in zip(p1,p2)]
       if pdg1==21:colours=[(101,102),(102,101),(0,0)]
       elif pdg2!=21:colours=[(101,0),(0,101),(0,0)]
       elif pdg1>0:colours=[(101,0),(102,101),(0,102)]
       else:colours=[(0,101),(101,102),(102,0)]
       records=[record(0,0,0,[0,0],[0]*4,0,0,0),record(11,21,0,[0,0],total,60,0,0),
        record(11,21,2,[4,5],total,60,0,0),record(4 if frozen else 1,pdg1,3,[0,0],p1,mass1,*colours[0]),
        record(1,pdg2,3,[0,0],p2,mass2,*colours[1]),record(4,-1 if pdg1>0 else 1,0,[0,0],[0,0,100,100],0,*colours[2])]
       sampling_types=[int(pdg1==21)^channel_swap,int(pdg2==21)^channel_swap]
       seed=2601000+len(commands)
       commands.append('BCONFIG\n3 1 .2 1 0\n'+('MPIINIT\n' if mpi_mode else 'HARDINIT\n')+row([maximum,1.2,-.7,1.3,20,angular])+(row([mpi_fsr,*sampling_types]) if mpi_mode else '')+''.join(row(r) for r in records)+'-1\n'+str(seed)+'\n')
       gap=abs(math.asinh(p1[2]/math.sqrt(mass1*mass1+p1[0]**2+p1[1]**2))-math.asinh(p2[2]/math.sqrt(mass2*mass2+p2[0]**2+p2[1]**2)))
       settings.append(dict(initial_rapidity_gap=gap,mpi_pair=mpi_mode,mpi_fsr=mpi_fsr,sampling_types=sampling_types,pdg1=pdg1,pdg2=pdg2,rapidity=rapidity,pt_hat=maximum,first_angle=angular,frozen=frozen,seed=seed))
payload='CONFIG\n208 3 1 .4 .59 .17 .17 .54 6.199999809265137 0 2 .45 1\n'+''.join(commands)
def run(exe):
 r=subprocess.run([exe],input=payload,text=True,capture_output=True,timeout=120)
 Path(output).with_suffix('.'+Path(exe).name+'.err').write_text(r.stderr)
 Path(output).with_suffix('.'+Path(exe).name+'.out').write_text(r.stdout)
 if r.returncode:raise RuntimeError((exe,r.returncode,r.stderr[-2000:]))
 rows=iter(r.stdout.splitlines());events=[]
 for _ in commands:events.append([list(map(float,next(rows).split())) for _ in range(7)])
 assert next(rows,None) is None
 return events,r.stderr
a,stderr=run(cpp);b,_=run(reference);failures=[];values=0;maximum=0
for case,(actual,expected) in enumerate(zip(a,b)):
 for i,(ar,br) in enumerate(zip(actual,expected)):
  assert len(ar)==len(br)
  for j,(x,y) in enumerate(zip(ar,br)):
   values+=1;maximum=max(maximum,abs(x-y))
   discrete=(i==0 and j==0) or (i>0 and (j<5 or 21<=j<=24))
   if not math.isfinite(x) or not math.isfinite(y) or not (x==y if discrete else math.isclose(x,y,rel_tol=2e-12,abs_tol=2e-13)):
    failures.append(dict(case=case,row=i,column=j,cpp=x,reference=y))
trials=[int(line.split()[1]) for line in stderr.splitlines() if line.startswith('HARD_INIT_TRIALS ')]
report=dict(scope='MPI-pair FINDVIRTUALITIES with explicit TYPE1/2 including mismatched pair flavours, no pair discovery' if mpi_mode else 'PPJJ FINDVIRTUALITIES and GETTHETAM, no MPI/EE/V+jet branches',cases=len(commands),values=values,
 max_absolute_error=maximum,max_draws=max(e[0][0] for e in b),retried_cases=sum(x>1 for x in trials),max_trials=max(trials),failures=failures,settings=settings,
 input_sha256=hashlib.sha256(payload.encode()).hexdigest(),sources=json.loads((Path(__file__).parents[1]/'reference/medium-source.json').read_text()),
 executables={Path(exe).name:hashlib.sha256(Path(exe).read_bytes()).hexdigest() for exe in [cpp,reference]})
Path(output).write_text(json.dumps(report,indent=2)+'\n')
print({k:v for k,v in report.items() if k not in ['failures','settings','sources','executables']})
if failures:print(failures[:25]);raise SystemExit(f'{len(failures)} hard initialization mismatches')
