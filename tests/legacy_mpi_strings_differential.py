"""Ordered MPI string topology, source tie handling and sequential insertion."""
import collections,hashlib,json,math,random,subprocess,sys
from pathlib import Path
cpp,reference,output=sys.argv[1:];rng=random.Random(2600421);cases=[]
def momentum(scale):
 p=[scale*rng.uniform(-1,1) for _ in range(3)];return p+[math.sqrt(sum(v*v for v in p))]
for i in range(2000):
 n=rng.randrange(2,21);scale=rng.choice([.01,1,10,1e5]);points=[momentum(scale) for _ in range(n+2)]
 pieces=[(j,j+1) for j in range(1,n)]
 if i%2:pieces.append((n,1))
 if i%5==0:pieces=[]
 flavour=rng.random();attach=rng.random();pdg=21 if flavour<attach or i%3 else rng.choice([1,2,3])
 cases.append((points,pieces,flavour,attach,pdg))
# Exact ties, duplicate geometry and both sides of the attachment threshold.
for scale in [.01,1,1e5]:
 for n in [0,2,8]:
  for shape in ['collinear','rest']:
   points=([0,0,scale,scale] if shape=='collinear' else [0,0,0,scale])
   points=[list(points) for _ in range(n+2)]
   pieces=[(j,j+1) for j in range(1,n)]
   for threshold in [0,.5,1]:
    for flavour in sorted(set([.1,.9]+[v for v in [math.nextafter(threshold,0),threshold,math.nextafter(threshold,1)] if sys.float_info.min<=v<1])):
     cases.append((points,pieces,flavour,threshold,21))
def encode(rows):
 text=''
 for points,pieces,flavour,attach,pdg in rows:
  text+=f'{len(points)-2} {len(pieces)} {flavour:.17g} {attach:.17g} {pdg}\n'
  text+=''.join(' '.join(format(x,'.17g') for x in p)+'\n' for p in points)
  text+=''.join(f'{a} {b}\n' for a,b in pieces)
 return text
payload=encode(cases)
def run(exe):
 r=subprocess.run([exe],input=payload,capture_output=True,text=True,timeout=30);assert r.returncode==0,(exe,r.stderr)
 rows=[list(map(int,line.split())) for line in r.stdout.splitlines()];assert len(rows)==len(cases)
 for row in rows:assert len(row)==1+2*row[0]
 return rows
a,b=run(cpp),run(reference);failures=[];integers=0;branches=collections.Counter()
for i,(left,right) in enumerate(zip(a,b)):
 integers+=len(left)
 if left!=right:failures.append(dict(case=i,cpp=left,fortran=right))
 points,pieces,f,attach,pdg=cases[i];n=len(points)-2;out=list(zip(left[1::2],left[2::2]))
 branch='insert' if f<attach and pieces else ('gluon-loop' if pdg==21 else 'quark-string');branches[branch]+=1
 assert len(out)==len(pieces)+(1 if branch=='quark-string' else 2)
 assert all(1<=x<=n+2 for edge in out for x in edge)
 if branch=='insert':
  for side in [0,1]:
   before=collections.Counter(e[side] for e in pieces);after=collections.Counter(e[side] for e in out)
   assert all(after[j]==before[j] for j in range(1,n+1))
   assert after[n+1]==after[n+2]==1
 else:
  assert out[:len(pieces)]==pieces
invalid=[]
base=([[0,0,1,1],[0,0,-1,1],[1,0,0,1],[-1,0,0,1]],[(1,2)],.2,.5,21)
for f,attach,pdg,pieces in [(-.1,.5,21,[(1,2)]),(1,.5,21,[(1,2)]),(.2,1.1,21,[(1,2)]),(.2,.5,22,[(1,2)]),(.2,.5,1,[(1,2)]),(.2,.5,21,[(1,3)]),(.2,.5,21,[(1,1)])]:
 invalid.append((base[0],pieces,f,attach,pdg))
for c in invalid:
 r=subprocess.run([cpp],input=encode([c]),capture_output=True,text=True);assert r.returncode!=0,c
report=dict(scope='ordered PYMULT KSTR topology only; no packed colour pointers or Pythia8 tag assignment',cases=len(cases),integer_comparisons=integers,branches=dict(branches),failures=failures,invalid_cases=len(invalid),input_sha256=hashlib.sha256(payload.encode()).hexdigest(),source_sha256='9fcfed664e4ac34462aa63679f2dd3e935c9f6f63f7cd6a785b73ccb82740ed9',executables={Path(e).name:hashlib.sha256(Path(e).read_bytes()).hexdigest() for e in [cpp,reference]})
Path(output).write_text(json.dumps(report,indent=2)+'\n');assert not failures,failures[:3]
print('PASS',len(cases),'cases;',integers,'exact integer comparisons;',dict(branches))
