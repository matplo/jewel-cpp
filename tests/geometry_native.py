import json,subprocess,math,sys,hashlib
from pathlib import Path
import tempfile
exe,ref,output=map(Path,sys.argv[1:]);exe=exe.resolve();ref=ref.resolve()
with tempfile.TemporaryDirectory() as tmp:
 outputs=[]
 for label,opts in [('off',[]),('on',['--geometry-step','.02'])]:
  p=Path(tmp)/(label+'.jsonl')
  r=subprocess.run([str(exe),str(p),'1','2601001','medium',*opts],capture_output=True,text=True,timeout=90)
  assert r.returncode==0,r.stderr
  outputs.append([json.loads(l) for l in p.read_text().splitlines()])
 baseline,rows=outputs;header=rows[0].pop('geometry_estimator');event=rows[1];geometry=event.pop('geometry')
 assert rows==baseline
 assert event['type']=='event'
 payload=f"CONFIG\n208 3 1 .4 .59 .17 .17 .54 6.199999809265137 {event['impact_parameter_fm']} 2 .45 1\n"
 expected=[]
 for g,p in zip(geometry,event['initial_hard']):
  assert g['shower_start_id']==p['id']
  args=[*g['production_fm'][:2],event['impact_parameter_fm'],header['radius_fm'],event['event_lifetime_fm'],*p['momentum'],header['step_fm']]
  payload+='LENGTHS\n'+' '.join(format(v,'.17g') for v in args)+'\n'
  expected.extend(g[k] for k in ['L_geom_T_fm','L_hot_fm','L_hot_T_fm','int_T3_dl'])
 result=subprocess.run([str(ref)],input=payload,capture_output=True,text=True,check=True)
 actual=list(map(float,result.stdout.split()));assert len(actual)==8
 assert all(math.isclose(x,y,abs_tol=2e-13,rel_tol=2e-12) for x,y in zip(actual,expected))
 invalid=0
 for opts in [['--geometry-step'],['--geometry-step','0'],['--geometry-step','-1'],['--geometry-step','nan'],['--geometry-step','inf'],['--geometry-step','0.02x'],['--geometry-step','.02','--geometry-step','.1']]:
  path=Path(tmp)/'invalid.jsonl'
  result=subprocess.run([str(exe),str(path),'1','2601001','medium',*opts],capture_output=True,timeout=15)
  assert result.returncode!=0 and not path.exists()
  invalid+=1
 report=dict(invalid_options_rejected=invalid,default_records_unchanged=True,geometry=geometry,max_reference_error=max(abs(x-y) for x,y in zip(actual,expected)),executables={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in [exe,ref]})
 output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
