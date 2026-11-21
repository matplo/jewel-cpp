import hashlib,json,math,random,struct,subprocess,sys
from pathlib import Path
cpp,fortran,output=sys.argv[1:]
commands=['EITABLE\n']; labels=['ei-table']*1000
# Query each reference interpolation knot and its adjacent representable values.
# Coordinates come from Fortran; table contents are still compared independently.
ei_rows=subprocess.run([fortran],input='EITABLE\n',text=True,capture_output=True,check=True).stdout.splitlines()
assert len(ei_rows)==1000
for row in ei_rows:
 knot=float(row.split()[0])
 for sign in [-1,1]:
  for x in [math.nextafter(sign*knot,-math.inf),sign*knot,math.nextafter(sign*knot,math.inf)]:
   if abs(x)<=55:
    commands.append(f'EI\n{x:.17g}\n'); labels.append('ei-knot-neighbours')
for x in [-55,-54.99,-40,-1,-.055,-.01,-1e-12,0,1e-12,.01,.055,.11,1,39.99,40,40.01,54.99,55]:
 commands.append(f'EI\n{x:.17g}\n'); labels.append('ei-interpolation')
for nf in [0,3,4,5,6]:
 for lam in [.1,.2,.3]:
  for q in [1,10,100,500]:
   for z in [.001,.1,.5,.9,.999]:
    for op in ['LOW','HIGH']:
     commands.append(f'{op}\n{nf} {z} {q} {lam} {lam}\n'); labels.append(op)
  for t in [.1,1,10,100,1000]:
   commands.append(f'SCAT\n{nf} {t} .5 {lam} {lam}\n'); labels.append('SCAT')
r=random.Random(2601)
# Cover light/heavy nuclei, default and altered thermodynamics, Woods-Saxon and flat profile.
for mass,nf,woods,b,taui,ti,tc in [(208,3,1,0,.4,.59,.17),(208,3,1,7,.4,.59,.17),(16,5,1,2,.6,.45,.15),(197,0,0,4,.3,.7,.12)]:
 commands.append(f'CONFIG\n{mass} {nf} {woods} {taui} {ti} {tc} .17 .54 6.199999809265137 {b} 2.0 .45 1.0\nTABLE\n')
 labels+=['thickness-table']*100
 # Radius and lookup upper edge use explicit REAL rounding, read reference table to avoid Python pow differences.
 prefix=''.join(commands)
 def table_from(exe):
  result=subprocess.run([exe],input=prefix,text=True,capture_output=True,check=True)
  return [list(map(float,line.split())) for line in result.stdout.splitlines()][-100:]
 knots=table_from(fortran)
 for knot,_ in knots:
  for x in [math.nextafter(knot,-math.inf),knot,math.nextafter(knot,math.inf)]:
   # The text probe does not accept subnormal decimal input; zero is tested.
   if x==0 or x>=sys.float_info.min:
    commands.append(f'THICK\n{x:.17g} 0\n'); labels.append('thickness-knot-neighbours')
 # Probe the overlap boundary in both nuclei at peak temperature, with a
 # flat profile so that a temperature cutoff cannot hide a geometric mismatch.
 commands.append(f'CONFIG\n{mass} {nf} 0 {taui} {ti} {tc} .17 .54 6.199999809265137 {b} 2.0 .45 1.0\n')
 for sign in [-1,1]:
  edge=sign*(knots[-1][0]-b/2)
  for x in [edge*(1-1e-10),math.nextafter(edge,-math.inf),edge,math.nextafter(edge,math.inf),edge*(1+1e-10)]:
   commands.append(f'POINT\n{x:.17g} 0 0 {taui:.17g}\n'); labels.append('overlap-transition')
 # Use the same finite spacetime coordinates for both engines; adjacent
 # rapidities need not remain adjacent after conversion through sinh/cosh.
 for sign in [-1,1]:
  edge=sign*2.
  for eta in [edge-sign*1e-10,math.nextafter(edge,-math.inf),edge,math.nextafter(edge,math.inf),edge+sign*1e-10]:
   commands.append(f'POINT\n0 0 {taui*math.sinh(eta):.17g} {taui*math.cosh(eta):.17g}\n'); labels.append('rapidity-transition')
 commands.append(f'CONFIG\n{mass} {nf} {woods} {taui} {ti} {tc} .17 .54 6.199999809265137 {b} 2.0 .45 1.0\n')
 commands.append('CROSS\n'); labels+=['cross-section-table']*200
 commands.append('BOUNDS\n'); labels.append('medium-bounds')
 for boost in [0,1]:
  commands.append(f'MAXIMUM\n{boost}\n'); labels.append('maximum-scatterer')
  for eta in [-3,-1,0,1,3]:
   commands.append(f'AVERAGE\n0 0 {math.sinh(eta):.17g} {math.cosh(eta):.17g} {boost}\n'); labels.append('average-scatterer')
 for radial in [0,knots[1][0],knots[50][0],knots[-1][0],knots[-1][0]+1e-10,20]:
  commands.append(f'THICK\n{radial:.17g} 0\n'); labels.append('thickness-boundary')
 for i in range(40):
  x,y,z,t=[r.uniform(-10,10) for _ in range(4)]
  commands.append(f'PART\n{x:.17g} {y:.17g} {z:.17g} {t:.17g}\n'); labels.append('participants')
 for tau in [.001,.1,taui*(1-1e-8),taui,taui*(1+1e-8),1,5,15,100]:
  for eta in [0,1,1.999999,2.000001,3]:
   for x,y in [(0,0),(3,1),(20,0)]:
    commands.append(f'POINT\n{x} {y} {tau*math.sinh(eta):.17g} {tau*math.cosh(eta):.17g}\n'); labels.append('medium-sample')
 for i in range(80):
  tau=r.uniform(.001,30);eta=r.uniform(-3,3);x=r.uniform(-10,10);y=r.uniform(-10,10)
  commands.append(f'POINT\n{x:.17g} {y:.17g} {tau*math.sinh(eta):.17g} {tau*math.cosh(eta):.17g}\n'); labels.append('medium-sample')
# Flat profile isolates the exact early growth / late cooling cutoff from
# Woods-Saxon normalization. Include adjacent doubles and safely bracketed points.
# JEWEL's cooling exponent is default REAL promoted to double, not exactly 1/3.
exponent=struct.unpack('f',struct.pack('f',.3333))[0]
for taui,ti,tc in [(.4,.59,.17),(.6,.45,.15),(.3,.7,.12)]:
 commands.append(f'CONFIG\n208 3 0 {taui} {ti} {tc} .17 .54 6.199999809265137 0 2 .45 1\n')
 for transition in [taui*tc/ti,taui,taui*(ti/tc)**(1/exponent)]:
  for tau in [transition*(1-1e-10),math.nextafter(transition,0),transition,math.nextafter(transition,math.inf),transition*(1+1e-10)]:
   commands.append(f'POINT\n0 0 0 {tau:.17g}\n'); labels.append('temperature-transition')
payload=''.join(commands)
def run(exe):
 result=subprocess.run([exe],input=payload,text=True,capture_output=True)
 if result.returncode: raise RuntimeError((exe,result.returncode,result.stderr))
 rows=[list(map(float,line.split())) for line in result.stdout.splitlines()]
 if len(rows)!=len(labels): raise AssertionError((exe,len(rows),len(labels),result.stderr))
 return rows
actual,expected=run(cpp),run(fortran)
summary={};failures=[]
for i,(label,a,b) in enumerate(zip(labels,actual,expected)):
 assert len(a)==len(b)
 group=summary.setdefault(label,{'values':0,'max_absolute_error':0.,'max_relative_error':0.})
 for col,(x,y) in enumerate(zip(a,b)):
  error=abs(x-y);rel=error/max(abs(y),1e-300)
  group['values']+=1;group['max_absolute_error']=max(group['max_absolute_error'],error);group['max_relative_error']=max(group['max_relative_error'],rel)
  activity_mismatch=label in ['medium-sample','temperature-transition','overlap-transition','rapidity-transition'] and col in [0,3] and ((x==0)!=(y==0))
  if activity_mismatch or not math.isfinite(x) or not math.isclose(x,y,rel_tol=2e-12,abs_tol=2e-13): failures.append({'row':i,'column':col,'label':label,'cpp':x,'fortran':y})
report={'scope':'optical medium and numerical tables, not full events','input_sha256':hashlib.sha256(payload.encode()).hexdigest(),
 'relative_tolerance':2e-12,'absolute_tolerance':2e-13,'groups':summary,'failures':failures,
 'sources':json.loads((Path(__file__).parents[1]/'reference/medium-source.json').read_text()),
 'executables':{Path(exe).name:hashlib.sha256(Path(exe).read_bytes()).hexdigest() for exe in [cpp,fortran]}}
Path(output).write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(summary,indent=2))
if failures: print(failures[:10]);raise SystemExit(f'{len(failures)} mismatches')
print('PASS',sum(g['values'] for g in summary.values()),'scalar comparisons')
