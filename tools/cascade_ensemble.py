"""Controlled parton-cascade ensemble; not full JEWEL/Pythia event validation."""
import argparse,hashlib,json,math,subprocess,time,platform
import importlib.metadata
from pathlib import Path
from cascade_cases import build_cases

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('cpp');parser.add_argument('reference');parser.add_argument('output')
parser.add_argument('--events',type=int,default=100)
parser.add_argument('--case',type=int,default=54,dest='case_index')
parser.add_argument('--seed-offset',type=int,default=1000000)
parser.add_argument('--coherence',choices=['fixture','off','on'],default='fixture')
parser.add_argument('--timeout',type=float,default=600)
args=parser.parse_args()
if args.events<2 or args.timeout<=0:parser.error('need at least two events and positive timeout')
from hepyy_utils.fastjet import require_fastjet
fj=require_fastjet()
analysis_versions={'python':platform.python_version(),'hepyy':importlib.metadata.version('hepyy'),
 'cppyy':importlib.metadata.version('cppyy'),'fastjet':str(fj.gbl.fastjet.fastjet_version_string()),
 'hepyy_utils_loader_sha256':hashlib.sha256(Path(require_fastjet.__code__.co_filename).read_bytes()).hexdigest()}
commands=[];settings=[]
for event in range(args.events):
 base,cases,labels,_=build_cases(guarded=True,generated=True,case_index=args.case_index,seed_offset=args.seed_offset+100*event,coherence=None if args.coherence=="fixture" else args.coherence=="on")
 if labels[0]['operation']!='GCASCADE':parser.error('ensemble requires a whole-cascade case')
 commands.extend(cases);settings.extend(labels)
payload=base+''.join(commands)
output=Path(args.output);output.parent.mkdir(parents=True,exist_ok=True)
binning={'pt':[5,10,20,30,40,50,60,70,80,100,150], 'girth':[0,.02,.04,.06,.08,.1,.15,.2,.3,.4], 'ptd':[0,.2,.3,.4,.5,.6,.7,.8,.9,1.0000001]}
def histogram(values,edges):
 result=[0]*(len(edges)-1)
 for x in values:
  for i,(a,b) in enumerate(zip(edges,edges[1:])):
   if a<=x<b:result[i]+=1;break
 return result

def analyze(rows):
 # Status 3 is a medium recoil; 4/5 are terminal shower partons.
 particles=fj.gbl.std.vector[fj.PseudoJet]()
 for row in rows:
  if int(row[0]) in [4,5]:particles.push_back(fj.PseudoJet(*row[5:9]))
 seq=fj.ClusterSequence(particles,fj.JetDefinition(fj.antikt_algorithm,.4));jets=[]
 for jet in fj.sorted_by_pt(seq.inclusive_jets(0)):
  if jet.pt()<5 or abs(jet.eta())>=2:continue
  constituents=list(jet.constituents());pts=[p.pt() for p in constituents];total=sum(pts)
  if total==0:continue
  girth=sum(p.pt()*math.hypot(p.eta()-jet.eta(),math.remainder(p.phi()-jet.phi(),2*math.pi)) for p in constituents)/total
  jets.append({'pt':jet.pt(),'eta':jet.eta(),'girth':girth,'ptd':math.sqrt(sum(p*p for p in pts))/total})
 return jets

def run(exe,tag):
 start=time.monotonic();raw=output.with_suffix('.'+tag+'.events');err=output.with_suffix('.'+tag+'.stderr')
 with raw.open('w') as stdout,err.open('w') as stderr:
  subprocess.run([exe],input=payload,text=True,stdout=stdout,stderr=stderr,timeout=args.timeout,check=True)
 elapsed=time.monotonic()-start;events=[];diagnostics=[];message=None
 if tag=='cpp':
  for line in err.read_text().splitlines():
   if line.startswith('COH_ERROR '):message=line.removeprefix('COH_ERROR ')
   if line.startswith('COH_STATS '):diagnostics.append(message);message=None
 else:
  for line in err.read_text().splitlines():
   if line.strip().startswith('GUARD_STATS'):
    reason=int(line.split()[1]);diagnostics.append({0:None,1:'nonfinite post-branch state',2:'branch limit exhausted'}[reason])
 if len(diagnostics)!=args.events:raise RuntimeError('missing per-event diagnostics')
 with raw.open() as stream:
  for event in range(args.events):
   header=list(map(float,next(stream).split()));n=int(header[0]);rows=[list(map(float,next(stream).split())) for _ in range(n)]
   for _ in range(167):next(stream)
   invalid=int(header[6]);discarded=bool(header[5]);accepted=not invalid and not discarded
   if accepted and any(not math.isfinite(x) for row in rows for x in row):raise RuntimeError('unflagged nonfinite event')
   if accepted and any(int(row[0]) in [1,2] for row in rows):raise RuntimeError('unfinished cascade flagged as accepted')
   jets=analyze(rows) if accepted else []
   events.append({'seed':settings[event]['seed'],'accepted':accepted,'diagnostic':diagnostics[event],'invalid':invalid,'discarded':discarded,
    'records':n,'draws':int(header[4]),'jets':jets,'histograms':{key:histogram([j[key] for j in jets],edges) for key,edges in binning.items()}})
  if stream.read().strip():raise RuntimeError('unexpected trailing output')
 return {'events':events,'seconds':elapsed,'accepted':sum(e['accepted'] for e in events),'selected_jets':sum(len(e['jets']) for e in events),
  'raw_sha256':hashlib.sha256(raw.read_bytes()).hexdigest(),'executable_sha256':hashlib.sha256(Path(exe).read_bytes()).hexdigest()}
cpp=run(args.cpp,'cpp');reference=run(args.reference,'reference')
comparisons={}
for key,edges in binning.items():
 bins=[]
 for index in range(len(edges)-1):
  a=[e['histograms'][key][index] for e in cpp['events']];b=[e['histograms'][key][index] for e in reference['events']]
  sa,sb=sum(a),sum(b);ratio=sa/sb if sb else None;error=None
  if sb:
   residual=[x-ratio*y for x,y in zip(a,b)]
   error=math.sqrt(sum(x*x for x in residual)/(args.events-1)/args.events)/(sb/args.events)
  bins.append({'low':edges[index],'high':edges[index+1],'cpp':sa,'reference':sb,'ratio':ratio,'paired_standard_error':error})
 comparisons[key]=bins
report={'scope':'controlled parton cascades only; no hard-event generation or hadronization','events_per_generator':args.events,
 'case':args.case_index,'rng':'splitmix64-open52-v1','settings':settings,'selection':{'algorithm':'anti-kt','R':.4,'min_pt':5,'max_abs_eta':2,'statuses':[4,5],'recoils':'status 3 excluded'},
 'normalization':'unit weight per attempted event; invalid/discarded events count as zero jets','uncertainty':'paired event-level delta method; empty reference bins have undefined ratios',
 'analysis_versions':analysis_versions,
 'full_goal_validated':False,'input_sha256':hashlib.sha256(payload.encode()).hexdigest(),
 'cpp':cpp,'reference':reference,'comparisons':comparisons,
 'sources':json.loads((Path(__file__).parents[1]/'reference/medium-source.json').read_text()),
 'guard_source_sha256':hashlib.sha256((Path(__file__).parents[1]/'reference/guarded_cascade.f90').read_bytes()).hexdigest()}
output.write_text(json.dumps(report,indent=2,allow_nan=False)+'\n')
print({tag:{k:v for k,v in value.items() if k not in ['events']} for tag,value in [('cpp',cpp),('reference',reference)]})
print('Saved',output)
