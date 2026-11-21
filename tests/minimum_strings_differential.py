"""Replay minimum-length strings against unchanged JEWEL 2.6.0 routines."""
import json,random,math,subprocess,sys,hashlib
from pathlib import Path
cpp,reference,output=map(Path,sys.argv[1:])
rng=random.Random(2602601);cases=[];labels=[]
def row(status,pdg,i,parent=0):
 px=rng.uniform(-20,20);py=rng.uniform(-20,20);pz=rng.uniform(-30,30)
 return [status,pdg,parent,0,0,px,py,pz,math.sqrt(px*px+py*py+pz*pz+.04),.2,1.,2.,3.,4.,5.,6.,7.,8.,9.,10.,100+i,200+i]
for i in range(60):
 rows=[row(17,0,0),row(17,0,1)]+[row([1,3,4,5][j%4],[1,-1,21,21,2,-2][(j+i)%6],j+2,1+j%2) for j in range(1+i%9)]
 if i%4==0:rows.insert(2,row(11,21,99))
 if i%3==0:
  a=row(4,22,0);a[-2:]=[0,0];rows.append(a)
 for compress in [0,1]:cases.append((rows,compress,2.));labels.append(f'mixed-{i}-compress-{compress}')
def add(rows,label,eta=2.):
 for compress in [0,1]:cases.append((rows,compress,eta));labels.append(f'{label}-compress-{compress}')
for pdg in [1,-1,21,2101,-2101]:
 add([row(4,pdg,1)],f'isolated-{pdg}')
for pz in [0,10,1e12]:
 r=row(4,21,1);r[5:10]=[0,0,pz,abs(pz),0]
 add([r],f'axial-gluon-{pz}')
for rapidity in [math.nextafter(3.,0),3.,math.nextafter(3.,math.inf),3.0001]:
 r=row(4,22,1);r[5:10]=[1,0,math.sinh(rapidity),math.cosh(rapidity),0];r[-2:]=[0,0]
 add([r],f'colourless-rapidity-{rapidity}')
for n in [22988,22990]:
 inactive=[17,0,0,0,0]+[0.]*15+[0,0]
 add([inactive[:] for _ in range(n-1)]+[row(4,21,1)],f'capacity-gluon-{n}')
for label,parent,px in [('same-parent',1,-10),('distinct-parent',2,-10),('collinear',2,10),('above-mass-cut',2,-20000)]:
 a=row(4,1,1,1);b=row(4,-1,2,parent)
 a[5:10]=[abs(px),0,0,abs(px),0];b[5:10]=[px,0,0,abs(px),0]
 add([row(17,0,0),row(17,0,1),a,b],label)
add([], 'empty')
tape=[rng.uniform(.001,.999) for _ in range(1000)]
payload=''.join(f'{len(rows)} {compress}\n{eta} {len(tape)}\n'+ ' '.join(map(str,tape))+'\n'+''.join(' '.join(format(v,'.17g') for v in r)+'\n' for r in rows) for rows,compress,eta in cases)

results=[]
for exe in [cpp,reference]:
 r=subprocess.run([str(exe.resolve()),'--minimum'],input=payload,text=True,capture_output=True,timeout=120)
 if r.returncode:raise RuntimeError((exe,r.returncode,r.stderr[-1500:]))
 lines=iter(r.stdout.splitlines());out=[]
 for c in cases:
  h=list(map(int,next(lines).split()));out.append((h,[list(map(float,next(lines).split())) for _ in range(h[0])]))
 assert next(lines,None) is None;results.append(out)
bad=[];maximum=0;values=0
for i,(a,b) in enumerate(zip(*results)):
 if a[0]!=b[0]:bad.append((i,'header',a[0],b[0]));continue
 for j,(ar,br) in enumerate(zip(a[1],b[1])):
  for k,(x,y) in enumerate(zip(ar,br)):
   values+=1;maximum=max(maximum,abs(x-y))
   if not math.isclose(x,y,abs_tol=2e-13,rel_tol=2e-12):bad.append((i,j,k,x,y))
manifest=json.loads((Path(__file__).resolve().parents[1]/'reference/medium-source.json').read_text())
report=dict(scope='literal MAKESTRINGS_MINL/CLEANUP and GENERATEZ; PYP dispatch limited to source J=17 expression',
 cases=len(cases),labels=labels,values=values,max_absolute_error=maximum,failures=bad,
 discarded_cases=sum(h[1] for h,_ in results[1]),random_draws=sum(h[2] for h,_ in results[1]),
 input_sha256=hashlib.sha256(payload.encode()).hexdigest(),source=manifest['jewel-2.6.0.f.stock']['sha256'],
 harness_sha256={name:hashlib.sha256((Path(__file__).resolve().parents[1]/'reference'/name).read_bytes()).hexdigest()
 for name in ['minimum_strings_support.f90','strings_probe.f90','extract_strings.py']},
 executables={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in [cpp,reference]})
output.write_text(json.dumps(report,indent=2)+'\n')
print('cases',len(cases),'values',values,'failures',len(bad),'maximum',maximum,'first',bad[:8])
if bad:raise SystemExit('minimum-string parity failed')
