"""Replay phase-tagged live ISUB=96 channel inputs through the native evaluator."""
import argparse,hashlib,json,math,subprocess
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('cpp',type=Path);p.add_argument('directory',type=Path);p.add_argument('baseline',type=Path);p.add_argument('output',type=Path);p.add_argument('--width-config',type=Path);p.add_argument('--pdf-probe',type=Path);a=p.parse_args()
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
records=[];current=None
for line in (a.directory/'run.log').read_text().splitlines():
 t=line.split()
 if not t or not t[0].startswith('CHANNEL_'):continue
 tag=t[0]
 if tag=='CHANNEL_CONTEXT':
  assert current is None
  current=dict(phase=int(t[1]),stage=int(t[2]),raw=[],weighted=[])
 elif tag=='CHANNEL_INPUT':assert len(t)==42;current['input']=' '.join(t[1:])+'\n'
 elif tag=='CHANNEL_PDF_CONTEXT':
  assert len(t)==16;current['pdf_context']=list(map(float,t[1:]))
 elif tag=='CHANNEL_RAW':current['raw'].append([*map(int,t[1:4]),float(t[4])])
 elif tag=='CHANNEL_TOTAL':current['count']=int(t[1]);current['total']=float(t[2])
 elif tag=='CHANNEL_WEIGHTED':
  current['weighted'].append(float(t[1]))
  if len(current['weighted'])==current['count']:
   assert len(current['raw'])==current['count'];records.append(current);current=None
assert current is None and len(records)==200
assert sum(x['phase']==0 for x in records)==sum(x['phase']==1 for x in records)==100
pdf_report=None
if a.pdf_probe:
 contexts=[r['pdf_context'] for r in records]
 floors=set(c[10] for c in contexts);assert len(floors)==1
 for record,c in zip(records,contexts):
  # PYPDFU reshapes only when MINT(35)>=2; MINT(31) can be nonzero
  # in legacy production without activating the companion model.
  assert c[11:14]==[13100,2,1] and record['stage']<2,(record['stage'],c)
 floor=next(iter(floors));rows=[]
 for c in contexts:
  for side in range(2):rows.append([c[side],c[2+side],c[4+side],c[6],c[7],c[8],c[9]])
 pdf_payload=''.join(' '.join(format(v,'.17g') for v in row)+'\n' for row in rows)
 pdf_run=subprocess.run([str(a.pdf_probe.resolve()),'CT14nlo','0',format(floor,'.17g')],input=pdf_payload,capture_output=True,text=True,check=True)
 pdf_values=[list(map(float,line.split())) for line in pdf_run.stdout.splitlines()]
 assert len(pdf_values)==2*len(records) and all(len(v)==11 for v in pdf_values)
 pdf_failures=[];pdf_maximum=0.
 for i,record in enumerate(records):
  fields=record['input'].split();native=pdf_values[2*i]+pdf_values[2*i+1]
  for j,(x,y) in enumerate(zip(native,map(float,fields[19:]))):
   pdf_maximum=max(pdf_maximum,abs(x-y))
   if not math.isfinite(x) or not math.isfinite(y) or not math.isclose(x,y,rel_tol=2e-12,abs_tol=2e-13) or ((x==0)!=(y==0)):
    pdf_failures.append(dict(case=i,column=j,cpp=x,fortran=y))
  record['input']=' '.join(fields[:19]+[format(v,'.17g') for v in native])+'\n'
 pdf_report=dict(scalar_comparisons=22*len(records),maximum_absolute_error=pdf_maximum,failures=pdf_failures,
  q2_floor=floor,set='CT14nlo/0',cpp_sha256=sha(a.pdf_probe),input_sha256=hashlib.sha256(pdf_payload.encode()).hexdigest(),
  evolution_stages=sorted(set(r['stage'] for r in records)),
  interaction_counters=sorted(set(int(c[14]) for c in contexts)),
  q2_range=[min(c[6] for c in contexts),max(c[6] for c in contexts)],
  remaining_fraction_range=[min(c[i] for c in contexts for i in [4,5]),max(c[i] for c in contexts for i in [4,5])])
payload=''.join(x['input'] for x in records)
r=subprocess.run([str(a.cpp.resolve())]+([str(a.width_config.resolve())] if a.width_config else []),input=payload,capture_output=True,text=True,check=True)
lines=iter(r.stdout.splitlines());failures=[];maximum=0.;compared=0
for i,record in enumerate(records):
 header=next(lines).split();assert int(header[0])==record['count'];pairs=[('total',float(header[1]),record['total'])]
 for j,raw in enumerate(record['raw']):
  row=next(lines).split();assert list(map(int,row[:3]))==raw[:3]
  pairs.extend([(f'{j}:raw',float(row[3]),raw[3]),(f'{j}:weighted',float(row[4]),record['weighted'][j])])
 for label,x,y in pairs:
  compared+=1;maximum=max(maximum,abs(x-y))
  if not math.isfinite(x) or not math.isfinite(y) or not math.isclose(x,y,rel_tol=2e-12,abs_tol=2e-13) or ((x==0)!=(y==0)):
   failures.append(dict(case=i,label=label,cpp=x,fortran=y))
assert next(lines,None) is None
identity=sha(a.directory/'events.hepmc')==sha(a.baseline)
report=dict(replay_tool_sha256=sha(Path(__file__)),scope='live ISUB=96 factors/PDF multiplication; native width if width_config_sha256 is present; native PDF evaluation if pdf_evaluation is present',pdf_evaluation=pdf_report,width_config_sha256=sha(a.width_config) if a.width_config else None,calls=len(records),initialization_calls=100,production_calls=100,channels=sum(x['count'] for x in records),numeric_comparisons=compared,maximum_absolute_error=maximum,failures=failures,hepmc_byte_identical=identity,baseline_hepmc_sha256=sha(a.baseline),input_sha256=hashlib.sha256(payload.encode()).hexdigest(),cpp_sha256=sha(a.cpp),artifacts={name:sha(a.directory/name) for name in ['pythia.f','core.f','trace.f90','reference','params.dat','medium.dat','run.log','instrumentation.patch']})
a.output.write_text(json.dumps(report,indent=2)+'\n');assert identity and not failures and (pdf_report is None or not pdf_report['failures']), (failures[:5],pdf_report)
print('PASS',compared,'live comparisons;',sum(x['count'] for x in records),'channels; byte-identical reference; max abs',maximum)
