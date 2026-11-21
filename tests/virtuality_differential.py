import hashlib,json,math,random,subprocess,sys
from pathlib import Path
cpp,reference,output=sys.argv[1:]
failures=[];groups={};records=[]
for nf,q0,lps,lqcd,boost,scale in [(3,1,.2,.2,0,1),(5,1.5,.25,.18,1,1.2)]:
 base=f'CONFIG\n208 {nf} 1 .4 .59 .17 .17 .54 6.199999809265137 3 2 .45 1\nFCONFIG\n{nf} {q0} {lps} 200 {scale} {boost}\nRCONFIG\n{nf} {q0} {lps} 200 {scale} {boost}\nXCONFIG\n{nf} {q0} {lps} {lqcd} 200 {scale} {boost}\n'
 commands=[];labels=[]
 for mode in range(6):
  commands.append(f'BCONFIG\n{nf} {q0} {lps} 1 {mode}\n')
  for pdg in [1,21]:
   for initial in [0,1]:
    for qnew,qold,previous,record in [(.5,5,2,2),(5,.5,0,0),(20,5,2,2),(3,10,2,2),(3,10,6,6),(10,10,2,2),(10,5,0,0),(3,10,0,0),(5*(1+5e-8),5,0,0),(5*(1+2e-7),5,0,0)]:
     for fraction in [.005,.5,1]:
      tape_rng=random.Random(9150+len(labels));tape=[tape_rng.uniform(.000001,.999999) for _ in range(1500)]
      values=[-qnew*qnew,-qold*qold,previous,record,100,pdg,initial,fraction,.37,1]
      commands.append('NEWMASS\n'+' '.join(format(x,'.17g') for x in values)+'\n1500\n'+' '.join(format(x,'.17g') for x in tape)+'\n')
      labels.append('initial-mass' if initial else 'final-mass')
 for pdg in [1,-1,21]:
  for momentum in [.1,2,10,100]:
   for mass in [0,.3,3]:
    for x,z,t in [(0,0,1),(3,.5,1),(3,-.5,1),(20,0,5)]:
     energy=math.sqrt(momentum*momentum+mass*mass)
     values=[momentum,0,0,energy,mass,pdg,x,0,z,t]
     commands.append('NORAD\n'+' '.join(format(x,'.17g') for x in values)+'\n');labels.append('no-initial-radiation')
 # Source returns finite values despite its transient 0/0 at zero shat.
 for pdg in [1,-1,21]:
  threshold=q0/scale
  for energy in [0,.1,math.nextafter(threshold,0),threshold,math.nextafter(threshold,math.inf),10]:
   for x,z,t in [(0,0,0),(2,0,0),(0,1,1),(0,-1,1),(0,2,1)]:
    values=[energy,0,0,energy,0,pdg,x,0,z,t]
    commands.append('NORAD\n'+' '.join(format(x,'.17g') for x in values)+'\n');labels.append('zero-invariant-no-radiation')
 payload=base+''.join(commands);rows=[]
 for exe in [cpp,reference]:
  result=subprocess.run([exe],input=payload,text=True,capture_output=True)
  if result.returncode:raise RuntimeError((exe,result.returncode,result.stderr[-2000:]))
  rows.append([list(map(float,line.split())) for line in result.stdout.splitlines()])
 assert len(rows[0])==len(rows[1])==len(labels)
 for index,(label,a,b) in enumerate(zip(labels,*rows)):
  assert len(a)==len(b)
  g=groups.setdefault(label,dict(cases=0,max_absolute_error=0.,max_relative_error=0.,max_draws=0))
  g['cases']+=1
  for col,(x,y) in enumerate(zip(a,b)):
   error=abs(x-y);g['max_absolute_error']=max(g['max_absolute_error'],error);g['max_relative_error']=max(g['max_relative_error'],error/max(abs(y),1e-300))
   equal=x==y if col>=2 else math.isclose(x,y,rel_tol=2e-12,abs_tol=2e-13)
   if not math.isfinite(x) or not equal:failures.append(dict(config=[nf,q0,boost],case=index,column=col,cpp=x,fortran=y))
  if len(b)==4:g['max_draws']=max(g['max_draws'],int(b[3]))
 records.append(dict(nf=nf,q0=q0,lambda_shower=lps,lambda_scattering=lqcd,boost=boost,scale=scale,cases=len(labels),input_sha256=hashlib.sha256(payload.encode()).hexdigest()))
report=dict(scope='GETNEWMASS and GETPNORAD1, not complete scattering evolution',groups=groups,failures=failures,records=records,relative_tolerance=2e-12,absolute_tolerance=2e-13,sources=json.loads((Path(__file__).parents[1]/'reference/medium-source.json').read_text()),executables={Path(exe).name:hashlib.sha256(Path(exe).read_bytes()).hexdigest() for exe in [cpp,reference]})
Path(output).write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(groups,indent=2))
if failures:print(failures[:10]);raise SystemExit('Virtuality parity failed')
