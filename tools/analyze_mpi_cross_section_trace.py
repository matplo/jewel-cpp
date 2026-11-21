"""Evaluate full native legacy MPI weights from captured proposal/settings only."""
import argparse,hashlib,json,math,subprocess
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('cpp',type=Path);p.add_argument('directory',type=Path);p.add_argument('baseline',type=Path);p.add_argument('output',type=Path);p.add_argument('--width-config',type=Path,required=True);p.add_argument('--alpha-s',type=float,required=True);a=p.parse_args()
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
records=[];current=None
for line in (a.directory/'run.log').read_text().splitlines():
 t=line.split()
 if not t or not t[0].startswith('CHANNEL_'):continue
 tag=t[0]
 if tag=='CHANNEL_CONTEXT':
  assert current is None;current=dict(phase=int(t[1]),stage=int(t[2]),raw=[],weighted=[])
 elif tag=='CHANNEL_INPUT':assert len(t)==42;current['input']=list(map(float,t[1:]))
 elif tag=='CHANNEL_PDF_CONTEXT':assert len(t)==16;current['pdf']=list(map(float,t[1:]))
 elif tag=='CHANNEL_NATIVE_CONTEXT':assert len(t)==18;current['native']=list(map(float,t[1:]))
 elif tag=='CHANNEL_RAW':current['raw'].append([*map(int,t[1:4]),float(t[4])])
 elif tag=='CHANNEL_TOTAL':current['count']=int(t[1]);current['total']=float(t[2])
 elif tag=='CHANNEL_WEIGHTED':
  current['weighted'].append(float(t[1]))
  if len(current['weighted'])==current['count']:
   assert len(current['raw'])==current['count'];records.append(current);current=None
assert current is None and len(records)==200
assert sum(r['phase']==0 for r in records)==sum(r['phase']==1 for r in records)==100
floors=set(r['pdf'][10] for r in records);assert len(floors)==1;floor=next(iter(floors));inputs=[]
for r in records:
 n,c,pdf=r['native'],r['input'],r['pdf']
 assert n[14:16]==[8,0] and pdf[11:14]==[13100,2,1] and r['stage']<2
 if r['phase']==1:assert n[16]==.5*n[0]*math.sqrt(n[6])
 inputs.append(n[:6]+[c[6],c[8],pdf[7],pdf[0],pdf[1]]+n[6:9]+[c[3],pdf[4],pdf[5],n[16] if r['phase']==0 else -1]+n[9:14]+c[9:19])
payload=''.join(' '.join(format(v,'.17g') for v in row)+'\n' for row in inputs)
run=subprocess.run([str(a.cpp.resolve()),'CT14nlo','0',str(a.width_config.resolve()),format(floor,'.17g'),format(a.alpha_s,'.17g')],input=payload,capture_output=True,text=True,timeout=180)
assert run.returncode==0,run.stderr[-2000:]
lines=iter(run.stdout.splitlines());groups={};failures=[]
def compare(group,x,y,case):
 g=groups.setdefault(group,dict(values=0,maximum_absolute_error=0.,maximum_relative_error=0.));g['values']+=1;g['maximum_absolute_error']=max(g['maximum_absolute_error'],abs(x-y));g['maximum_relative_error']=max(g['maximum_relative_error'],abs(x-y)/max(abs(y),1e-300))
 if not math.isfinite(x) or not math.isfinite(y) or not math.isclose(x,y,rel_tol=2e-12,abs_tol=2e-13) or ((x==0)!=(y==0)):
  if len(failures)<30:failures.append(dict(group=group,case=case,cpp=x,fortran=y))
for i,r in enumerate(records):
 values=list(map(float,next(lines).split()));assert len(values)==31 and int(values[29])==r['count']
 c=r['input'];expected=c[:3]+[r['pdf'][6],c[5],c[4],c[7]]+c[19:]
 for j,(x,y) in enumerate(zip(values[:29],expected)):
  group='kinematics' if j<4 else ('alpha_s' if j==4 else 'common-factor' if j==5 else 'gluon-width' if j==6 else 'pdfs')
  compare(group,x,y,i)
 compare('channel-totals',values[30],r['total'],i)
 for j,raw in enumerate(r['raw']):
  row=next(lines).split();assert list(map(int,row[:3]))==raw[:3]
  compare('raw-channel-weights',float(row[3]),raw[3],i)
  compare('pdf-weighted-channels',float(row[4]),r['weighted'][j],i)
assert next(lines,None) is None
identity=sha(a.directory/'events.hepmc')==sha(a.baseline)
report=dict(scope='native legacy MPI cross section from proposal/settings with Pythia8 AlphaStrong and LHAPDF; captured proposals, no native event distribution',calls=len(records),initialization_calls=100,production_calls=100,groups=groups,failures=failures,hepmc_byte_identical=identity,baseline_hepmc_sha256=sha(a.baseline),pdf_set='CT14nlo/0',q2_floor=floor,alpha_s_mz=a.alpha_s,alpha_s_order=1,alpha_s_nfmax=5,cmw=False,input_sha256=hashlib.sha256(payload.encode()).hexdigest(),cpp_sha256=sha(a.cpp),width_config_sha256=sha(a.width_config),tool_sha256=sha(Path(__file__)),relative_tolerance=2e-12,absolute_tolerance=2e-13,artifacts={name:sha(a.directory/name) for name in ['pythia.f','core.f','trace.f90','reference','params.dat','medium.dat','run.log','instrumentation.patch']})
a.output.write_text(json.dumps(report,indent=2)+'\n');assert identity and not failures,failures[:5]
print('PASS',sum(g['values'] for g in groups.values()),'native cross-section comparisons; byte-identical reference')
