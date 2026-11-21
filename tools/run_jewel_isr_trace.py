"""Run the isolated vacuum ISR trace and prove baseline event/RNG identity."""
import argparse,collections,csv,hashlib,json,math,os,re,subprocess
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('build',type=Path)
p.add_argument('card',type=Path)
p.add_argument('output',type=Path,help='New sample directory')
p.add_argument('--lhapdf-data',required=True,type=Path)
p.add_argument('--events',type=int,default=1000)
a=p.parse_args()
if a.events<=0:p.error('--events must be positive')
build=a.build.resolve();card=a.card.resolve();out=a.output.resolve()
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
manifest=json.loads((build/'manifest.json').read_text())
for name,digest in manifest['artifacts'].items():
 if sha(build/name)!=digest:raise ValueError(f'Build artifact changed: {name}')
text=card.read_text()
def set_value(key,value):
 global text
 pattern=rf'(?m)^{key}\s+[^\n]*$'
 if len(re.findall(pattern,text))!=1:raise ValueError(f'Expected one {key} in card')
 text=re.sub(pattern,lambda _:f'{key} {value}',text)
inputs={}
for key in ['SPLITINTFILE','PDFFILE','XSECFILE','MEDIUMPARAMS']:
 match=re.findall(rf'(?m)^{key}\s+(\S+)\s*$',text)
 if len(match)!=1:raise ValueError(f'Expected one {key} file')
 path=(card.parent/match[0]).resolve();inputs[key]=dict(path=str(path),sha256=sha(path))
 set_value(key,str(path))
set_value('NEVENT',a.events);set_value('LOGFILE','jewel.log');set_value('HEPMCFILE','events.hepmc')
out.mkdir(exist_ok=False)
env=os.environ.copy();env['LHAPDF_DATA_PATH']=str(a.lhapdf_data.resolve())
for mode in ['trace','baseline']:
 directory=out/mode;directory.mkdir();(directory/'params.dat').write_text(text)
 with (directory/'stdout.log').open('w') as log:
  subprocess.run([str(build/('jewel-vac-'+mode)),'params.dat'],cwd=directory,env=env,stdout=log,stderr=subprocess.STDOUT,check=True)
 print(mode,'finished',flush=True)
identity={}
for name in ['events.hepmc','out.ran']:
 if (out/'trace'/name).read_bytes()!=(out/'baseline'/name).read_bytes():
  raise AssertionError(f'Instrumentation changed {name}')
 identity[name]=sha(out/'trace'/name)
events=sum(line.startswith('E ') for line in (out/'trace/events.hepmc').open())
if events!=a.events:raise AssertionError((events,a.events))
rows=list(csv.DictReader((out/'trace/isr-lines.csv').open()))
if not rows:raise AssertionError('Empty trace')
groups={};attempts=collections.defaultdict(list);nonfinite_times=[];checked_incoming=0
for row in rows:
 role=int(row['role']);energy=float(row['energy']);start=float(row['production_time']);end=float(row['end_time'])
 if role not in [1,2,3]:raise AssertionError(row)
 if not all(math.isfinite(float(row[k])) for k in ['px','py','pz','energy','mass']):raise AssertionError(row)
 finite_time=all(math.isfinite(float(row[k])) for k in ['x','y','z','production_time','end_time'])
 if not finite_time:nonfinite_times.append(row)
 terminal=role==2 and int(row['parent']) in [3,4]
 label={1:'hard_incoming',2:'ancestor_incoming',3:'emitted'}[role]
 if terminal:label='terminal_beam'
 g=groups.setdefault(label,dict(lines=0,negative_energy=0,zero_energy=0,reversed_intervals=0,zero_intervals=0,nonfinite_time_records=0,min_energy=energy,max_energy=energy))
 g['nonfinite_time_records']+=not finite_time
 g['lines']+=1;g['negative_energy']+=energy<0;g['zero_energy']+=energy==0
 g['reversed_intervals']+=start>end;g['zero_intervals']+=start==end
 g['min_energy']=min(g['min_energy'],energy);g['max_energy']=max(g['max_energy'],energy)
 if role in [1,2] and not terminal and finite_time and start!=end:
  checked_incoming+=1
  if (start>end)!=(energy<0):raise AssertionError(('Incoming energy/time sign mismatch',row))
 if role==1:attempts[int(row['attempt'])].append(int(row['side']))
if any(sorted(sides)!=[0,1] for sides in attempts.values()):raise AssertionError('Incomplete hard pair')
stages={}
for row in csv.DictReader((out/'trace/isr-stages.csv').open()):
 stage=int(row['stage'])
 if stage not in [1,2]:raise AssertionError(row)
 g=stages.setdefault(str(stage),dict(records=0,active_records=0,nonfinite_records=0,
   active_nonfinite_records=0,attempts=set(),nonfinite_examples=[]))
 g['records']+=1;g['attempts'].add(int(row['attempt']))
 active=int(row['status']) in [1,2];g['active_records']+=active
 finite=all(math.isfinite(float(row[k])) for k in ['x','y','z','production_time','end_time'])
 g['nonfinite_records']+=not finite;g['active_nonfinite_records']+=active and not finite
 if not finite and len(g['nonfinite_examples'])<10:g['nonfinite_examples'].append(row)
for g in stages.values():
 if g['attempts']!=set(attempts):raise AssertionError('Missing downstream trace stage')
 g['attempts']=len(g['attempts'])
if set(stages)!={'1','2'}:raise AssertionError('Missing stage')
report=dict(scope='Actual stock Pythia6 histories at JEWEL ISR timing; not Pythia8 equivalence',
 downstream_stages=stages,stage_trace_sha256=sha(out/'trace/isr-stages.csv'),
 accepted_events=events,interpretation_attempts=len(attempts),trace_rows=len(rows),groups=groups,
 finite_incoming_energy_time_sign_agreement=True,finite_incoming_sign_checks=checked_incoming,
 nonfinite_time_records=nonfinite_times,baseline_byte_identity_sha256=identity,
 trace_sha256=sha(out/'trace/isr-lines.csv'),parameter_card=text,input_files=inputs,
 build_manifest=manifest,runner_sha256=sha(Path(__file__)))
(out/'validation.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(groups,indent=2));print('PASS',events,'baseline-identical events')
