"""Literal MAKEMASSLESS comparison, including sequential component updates."""
import hashlib,json,math,subprocess,sys
from pathlib import Path
cpp,reference,output=sys.argv[1:]
def boost(p,b):
 b2=sum(x*x for x in b)
 if not b2:return p
 gamma=1/math.sqrt(1-b2);dot=sum(x*y for x,y in zip(b,p[:3]))
 factor=(gamma-1)*dot/b2+gamma*p[3]
 return [p[i]+factor*b[i] for i in range(3)]+[gamma*(p[3]+dot)]
cases=[];commands=[]
for scale in [10,100]:
 for masses in [(0,0),(1.5,4.8)]:
  for emitted_mass in [0,.13957,4.8]:
   for direction in [(1,0,0),(0,1,0),(0,0,1),(1,2,3)]:
    norm=math.sqrt(sum(x*x for x in direction));p=[scale*x/norm for x in direction]
    for beta in [(0,0,0),(.3,0,0),(0,0,.8),(.1,-.2,.3)]:
     rest=[p+[math.sqrt(scale*scale+masses[0]**2)],[-x for x in p]+[math.sqrt(scale*scale+masses[1]**2)],
       [1.,-2.,3.,math.sqrt(14+emitted_mass**2)]]
     records=[]
     for i,(mom,mass) in enumerate(zip(rest,[*masses,emitted_mass])):
      records.append(boost(mom,beta)+[mass]+[.1*(i+1),-.2,.3,1.])
     commands.append('MASSLESS\n'+''.join(' '.join(format(x,'.17g') for x in row)+'\n' for row in records))
     cases.append(dict(scale=scale,masses=masses,emitted_mass=emitted_mass,direction=direction,beta=beta,records=records))
payload=''.join(commands);results=[]
for exe in [cpp,reference]:
 run=subprocess.run([exe],input=payload,text=True,capture_output=True,check=True,timeout=90)
 results.append([float(x) for x in run.stdout.split()])
a,b=results;assert len(a)==len(b)==len(cases)*27
maximum=0;failures=[];max_hard_shell_residual=0;max_closure=0
for i,case in enumerate(cases):
 actual=a[i*27:(i+1)*27];expected=b[i*27:(i+1)*27]
 for j,(x,y) in enumerate(zip(actual,expected)):
  maximum=max(maximum,abs(x-y))
  if not math.isfinite(x) or not math.isclose(x,y,rel_tol=2e-12,abs_tol=2e-13):failures.append(dict(case=i,component=j,cpp=x,reference=y))
 assert actual[22]==0 # third record mass
 for base in [0,9]:
  p=actual[base:base+5]
  max_hard_shell_residual=max(max_hard_shell_residual,abs(p[3]**2-sum(x*x for x in p[:3])-p[4]**2))
 for component in range(4):
  max_closure=max(max_closure,abs(sum(actual[base+component] for base in [0,9,18])-sum(row[component] for row in case['records'])))
report=dict(scope='Literal MAKEMASSLESS, not complete ISR event preparation',cases=len(cases),values=len(a),
 max_absolute_error=maximum,relative_tolerance=2e-12,absolute_tolerance=2e-13,failures=failures,
 max_hard_mass_shell_residual_gev2=max_hard_shell_residual,max_total_four_momentum_residual_gev=max_closure,
 input_sha256=hashlib.sha256(payload.encode()).hexdigest(),
 executables={Path(exe).name:hashlib.sha256(Path(exe).read_bytes()).hexdigest() for exe in [cpp,reference]},
 sources=json.loads((Path(__file__).parents[1]/'reference/kinematics-source.json').read_text()))
Path(output).write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
if failures:raise SystemExit('MAKEMASSLESS differential failure')
