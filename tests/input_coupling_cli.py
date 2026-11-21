"""Explicit ISR coupling policy reaches initialized Pythia and event runners."""
import hashlib,json,subprocess,sys,tempfile
from pathlib import Path
hard,events,output=sys.argv[1:]
alpha='0.11754434294459773'
policy=['--hard-alpha-s',alpha,'--hard-alpha-s-order','1','--isr-alpha-s',alpha,'--isr-alpha-s-order','1','--alpha-s-nfmax','5','--isr-alpha-s-cmw','off','--mpi-alpha-s',alpha,'--mpi-alpha-s-order','1','--mpi-screening','2','1800','.25','--mpi-double-gaussian','.5','.4']
records=[]
with tempfile.TemporaryDirectory() as directory:
 root=Path(directory)
 for exe,mode,key in [(hard,['--isr','on','--mpi','on'],'settings'),(events,['vacuum','--experimental-isr-mpi'],'pythia_settings')]:
  out=root/(Path(exe).name+'.jsonl')
  result=subprocess.run([exe,str(out),'3','2600421',*mode,*policy],capture_output=True,text=True,timeout=180)
  assert result.returncode==0,result.stderr[-2000:]
  rows=[json.loads(line) for line in out.read_text().splitlines()]
  settings=dict(s.split(' = ',1) for s in rows[0][key])
  for name in ['SigmaProcess','SpaceShower','MultipartonInteractions']:
   assert float(settings[name+':alphaSvalue'])==float(alpha)
   assert settings[name+':alphaSorder']=='1'
  assert settings['MultipartonInteractions:pT0parametrization']=='0'
  assert [float(settings['MultipartonInteractions:'+k]) for k in ['pT0Ref','ecmRef','ecmPow']]==[2,1800,.25]
  assert settings['MultipartonInteractions:bProfile']=='2'
  assert float(settings['MultipartonInteractions:coreFraction'])==.5
  assert float(settings['MultipartonInteractions:coreRadius'])==.4
  assert settings['StandardModel:alphaSnfmax']=='5'
  assert settings['SpaceShower:alphaSuseCMW']=='off'
  assert sum(row['type']=='event' for row in rows)==3
  records.append(dict(executable=Path(exe).name,events=3,settings=settings,output_sha256=hashlib.sha256(out.read_bytes()).hexdigest()))
 invalid=[['--mpi-double-gaussian','.5'],['--mpi-double-gaussian','nan','.4'],['--mpi-double-gaussian','.5','inf'],['--mpi-double-gaussian','1.1','.4'],['--mpi-double-gaussian','-.1','.4'],['--mpi-double-gaussian','.5','.09'],['--mpi-double-gaussian','.5','1.1'],['--mpi-double-gaussian','.5x','.4'],['--mpi-double-gaussian','.5','.4','--mpi-double-gaussian','.5','.4'],['--mpi-alpha-s',alpha],['--mpi-alpha-s-order','1'],['--mpi-alpha-s','nan','--mpi-alpha-s-order','1'],['--mpi-alpha-s',alpha,'--mpi-alpha-s-order','4'],['--mpi-alpha-s',alpha,'--mpi-alpha-s',alpha,'--mpi-alpha-s-order','1'],['--mpi-screening','2','1800'],['--mpi-screening','.1','1800','.25'],['--mpi-screening','2','nan','.25'],['--mpi-screening','2','1800','.6'],['--mpi-screening','2','1800','.25','--mpi-screening','2','1800','.25'],['--isr-alpha-s',alpha],['--isr-alpha-s-order','1'],
  ['--isr-alpha-s','nan','--isr-alpha-s-order','1'],
  ['--isr-alpha-s','.05','--isr-alpha-s-order','1'],
  ['--isr-alpha-s',alpha,'--isr-alpha-s-order','4'],
  ['--isr-alpha-s','0.12bad','--isr-alpha-s-order','1'],
  ['--alpha-s-nfmax','4'],['--alpha-s-nfmax','5x'],
  ['--isr-alpha-s-cmw','false'],['--isr-alpha-s-cmw'],
  ['--isr-alpha-s-cmw','off','--isr-alpha-s-cmw','on'],
  ['--alpha-s-nfmax','5','--alpha-s-nfmax','6'],
  ['--isr-alpha-s',alpha,'--isr-alpha-s',alpha,'--isr-alpha-s-order','1'],
  ['--isr-alpha-s',alpha,'--isr-alpha-s-order','1','--isr-alpha-s-order','2']]
 for exe,mode in [(hard,[]),(events,['vacuum'])]:
  for i,args in enumerate(invalid):
   out=root/(Path(exe).name+str(i)+'.jsonl')
   result=subprocess.run([exe,str(out),'1','17',*mode,*args],capture_output=True,text=True,timeout=30)
   assert result.returncode!=0 and not out.exists(),args
 # Exercise the other valid boolean/cap, independent of value/order overrides.
 out=root/'alternate.jsonl'
 subprocess.run([hard,str(out),'1','17','--isr','on','--isr-alpha-s-cmw','on','--alpha-s-nfmax','6'],check=True,capture_output=True,text=True,timeout=90)
 settings=json.loads(out.read_text().splitlines()[0])['settings']
 assert 'SpaceShower:alphaSuseCMW = on' in settings and 'StandardModel:alphaSnfmax = 6' in settings
report=dict(scope='runner coupling wiring and post-initialization verification; not jet parity',valid_runs=records,invalid_cases=2*len(invalid),alternate_policy_passed=True,
 executables={Path(exe).name:hashlib.sha256(Path(exe).read_bytes()).hexdigest() for exe in [hard,events]})
Path(output).write_text(json.dumps(report,indent=2)+'\n')
print('PASS: two three-event runs, alternate policy and',2*len(invalid),'invalid inputs')
