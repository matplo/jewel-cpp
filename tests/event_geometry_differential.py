"""Change event geometry after tabulation, as in MEDNEXTEVT/GENEVENT."""
import hashlib,json,math,subprocess,sys,tempfile
from pathlib import Path
cpp,reference,output=sys.argv[1:];reports=[];failures=[];values=0;maximum=0
with tempfile.TemporaryDirectory() as directory:
 for table_b,boost,energy in [(b,boost,e) for b,boost in [(0,0),(7,1)] for e in [100,1.5*150*math.cosh(2),2510]]:
  base=f'CONFIG\n208 3 1 .4 .59 .17 .17 .54 6.199999809265137 {table_b} 2 .45 1\nXCONFIG\n3 1 .2 .2 {energy:.17g} 1 {boost}\n'
  results=[]
  for exe in [cpp,reference]:
   before=Path(directory)/(Path(exe).name+'.before');after=Path(directory)/(Path(exe).name+'.after')
   payload=base+f'XDUMP\n{before}\n'
   for b in [0,3,9,14,table_b]:
    payload+=f'EVENTB\n{b}\nBOUNDS\n'
    for x in [0,3,7]:
     for time in [.2,1,4]:
      # Nonzero spacetime rapidity exercises flow after a geometry update;
      # z=0 alone cannot distinguish boost on/off. Include both hemispheres.
      for eta in [-1.5,0,1.5]:
       z=time*math.sinh(eta);t=time*math.cosh(eta)
       payload+=f'POINT\n{x} 0 {z:.17g} {t:.17g}\n'
       for incoming in ['Q','G']:
        for mode in [0,1,2]:payload+=f'XSEC\n10 0 0 10 0 0 {incoming} C {x} 0 {z:.17g} {t:.17g} {mode}\n'
    for channel in ['QA','QB','GA','GB']:payload+=f'XINT\n{channel} 10 .8\n'
   payload+=f'XDUMP\n{after}\n'
   result=subprocess.run([exe],input=payload,text=True,capture_output=True,timeout=90)
   if result.returncode:raise RuntimeError((exe,result.stderr[-1000:]))
   assert before.read_bytes()==after.read_bytes(),'event geometry rebuilt initialized tables'
   reports.append(dict(executable=Path(exe).name,table_b=table_b,boost=boost,table_energy_gev=energy,
     unchanged_table_sha256=hashlib.sha256(before.read_bytes()).hexdigest()))
   results.append([float(v) for v in result.stdout.split()])
  a,b=results;assert len(a)==len(b)
  for i,(x,y) in enumerate(zip(a,b)):
   values+=1;maximum=max(maximum,abs(x-y))
   if not math.isfinite(x) or not math.isfinite(y) or not math.isclose(x,y,rel_tol=2e-12,abs_tol=2e-13):failures.append(dict(table_b=table_b,table_energy_gev=energy,index=i,cpp=x,reference=y))
report=dict(scope='event-local medium/rates after frozen initialization tables',values=values,max_absolute_error=maximum,
 failures=failures,reports=reports,relative_tolerance=2e-12,absolute_tolerance=2e-13,
 sources=json.loads((Path(__file__).parents[1]/'reference/medium-source.json').read_text()),
 executables={Path(p).name:hashlib.sha256(Path(p).read_bytes()).hexdigest() for p in [cpp,reference]})
Path(output).write_text(json.dumps(report,indent=2)+'\n');print(values,maximum,len(failures))
if failures:print(failures[:5]);raise SystemExit('event geometry mismatch')
