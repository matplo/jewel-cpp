"""Native hadronized samples via hepyy/cppyy FastJet; not Fortran validation."""
import argparse
from collections import Counter
import hashlib
import importlib.metadata
import json
import math
from pathlib import Path
import platform
from jet_substructure import soft_drop, SOFT_DROP
from native_geometry_analysis import associate_geometry, match_hard_legs

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('inputs',nargs='+',type=Path)
parser.add_argument('--output',required=True,type=Path)
parser.add_argument('--generator',type=Path,help='Executable used to produce the supplied files')
args=parser.parse_args()
from hepyy_utils.fastjet import require_fastjet
fj=require_fastjet()
bins={'mass':[-50,0,1,2,4,6,8,12,20,40,80],
      'groomed_mass':[-50,0,1,2,4,6,8,12,20,40,80],
      'zg':[.1,.15,.2,.25,.3,.35,.4,.45,.5000000000000001],
      'pt':[10,15,20,30,40,60,80,100,150,250],
      'girth':[0,.02,.04,.06,.08,.1,.15,.2,.3,.4],
      'ptd':[0,.2,.3,.4,.5,.6,.7,.8,.9,1.0000001]}

def histogram(values,edges):
    result=[0]*(len(edges)-1)
    for value in values:
        if value is None:continue
        if not math.isfinite(value):raise ValueError("nonfinite observable")
        for i,(lo,hi) in enumerate(zip(edges,edges[1:])):
            if lo<=value<hi:
                result[i]+=1
                break
    return result

def analyze(path):
    events=[];failures=Counter();summary=None;header=None
    digest=hashlib.sha256()
    with path.open('rb') as source:
        for raw in source:
            digest.update(raw);record=json.loads(raw)
            if record['type']=='run':
                assert header is None
                header=record
                assert header['schema'] in ['jewel-hadron-development-v1','jewel-fortran-hepmc-analysis-v1']
            elif record['type']=='summary':
                assert summary is None
                summary=record
            else:
                assert header is not None and summary is None
                assert record['attempt']==len(events)+1
                geometry_matches=None
                jets=[];accepted=record['type']=='event';weight=record.get('weight',0.)
                if accepted:
                    particles=fj.gbl.std.vector[fj.PseudoJet]()
                    collection='final_particles' if header['schema']=='jewel-fortran-hepmc-analysis-v1' else 'projected_particles' if header.get('subtraction',{}).get('put_on_shell',0) else 'subtracted_particles' if header.get('subtraction') else 'hadronized_particles'
                    for p in record[collection]:
                        if not p['final'] or abs(p['pdg']) in [12,14,16]:continue
                        assert all(math.isfinite(v) for v in p['momentum'])
                        particles.push_back(fj.PseudoJet(*p['momentum']))
                    # No constituent acceptance cut: cluster first in full phase space.
                    sequence=fj.ClusterSequence(particles,fj.JetDefinition(fj.antikt_algorithm,.4))
                    all_jets=[j for j in fj.sorted_by_pt(sequence.inclusive_jets(0)) if j.pt()>0]
                    geometry_by_jet={}
                    if 'geometry' in record:
                        if header.get('geometry_estimator',{}).get('schema')!='geometry-v1-native':
                            raise ValueError('unsupported native geometry schema')
                        axes=[dict(jet_id=i,jet_rank=i+1,eta=j.eta(),phi=j.phi(),pt=j.pt()) for i,j in enumerate(all_jets)]
                        geometry_matches,geometry_by_jet=associate_geometry(record,axes)
                    for jet_id,jet in enumerate(all_jets):
                        if jet.pt()<10 or abs(jet.eta())>=2:continue
                        constituents=list(jet.constituents());total=math.fsum(p.pt() for p in constituents)
                        if total<=0:continue
                        girth=math.fsum(p.pt()*math.hypot(p.eta()-jet.eta(),math.remainder(p.phi()-jet.phi(),2*math.pi)) for p in constituents)/total
                        ptd=math.sqrt(math.fsum(p.pt()**2 for p in constituents))/total
                        jets.append(dict(pt=jet.pt(),eta=jet.eta(),mass=jet.m(),girth=girth,ptd=ptd,
                                         **soft_drop(jet,fj)))
                        if geometry_matches is not None:
                            jets[-1].update(jet_id=jet_id,jet_rank=jet_id+1,**geometry_by_jet.get(jet_id,
                                dict(matched_leg_id=None,matching_delta_r=None,matching_ambiguous=None,geometry=None)))
                else:
                    failures[record.get('stage','generation')+': '+record.get('reason','generation failure')]+=1
                events.append(dict(attempt=record['attempt'],accepted=accepted,weight=weight,jets=jets,
                    histograms={key:histogram([j[key] for j in jets],edges) for key,edges in bins.items()}))
                if geometry_matches is not None:events[-1]['hard_leg_matches']=geometry_matches
    assert summary is not None and summary['attempts']==len(events)==header['attempts_requested']
    assert summary['accepted']==sum(e['accepted'] for e in events)
    assert math.isclose(sum(e['weight'] for e in events),summary['generator_weight_sum'],rel_tol=1e-12)
    distributions={}
    for key,edges in bins.items():
        distributions[key]=[]
        for i in range(len(edges)-1):
            values=[e['weight']*e['histograms'][key][i] for e in events]
            mean=math.fsum(values)/len(events)
            error=math.sqrt(math.fsum((v-mean)**2 for v in values)/(len(events)-1)/len(events)) if len(events)>1 else None
            distributions[key].append(dict(low=edges[i],high=edges[i+1],weighted_count=math.fsum(values),
                jets_per_attempt=mean,event_standard_error=error))
    # Account explicitly for untagged and out-of-range entries, including weights.
    accounting={}
    for key,edges in bins.items():
        counts=Counter();weighted=Counter()
        for event in events:
            for jet in event['jets']:
                v=jet[key]
                category='undefined' if v is None else 'underflow' if v<edges[0] else 'overflow' if v>=edges[-1] else 'in_range'
                counts[category]+=1;weighted[category]+=event['weight']
        accounting[key]={category:dict(jets=counts[category],weighted_count=weighted[category])
                         for category in ['in_range','underflow','overflow','undefined']}
    return dict(denominator_scope=header.get('denominator_scope','attempted-events'),observable_accounting=accounting,input=str(path),input_sha256=digest.hexdigest(),run=header,summary=summary,
        selected_jets=sum(len(e['jets']) for e in events),failure_reasons=dict(failures),distributions=distributions,events=events)

samples=[analyze(path) for path in args.inputs]
for sample in samples:
    geometry_events=[e for e in sample['events'] if 'hard_leg_matches' in e]
    if geometry_events:
        legs=[m for e in geometry_events for m in e['hard_leg_matches']]
        selected=[j for e in geometry_events for j in e['jets']]
        sample['geometry_accounting']=dict(events=len(geometry_events),hard_legs=len(legs),
            matched_legs=sum(m['matched'] for m in legs),
            matched_jets_passing_analysis=sum(m['jet_passes_analysis'] for m in legs),
            matched_jets_outside_analysis=sum(m['matched'] and not m['jet_passes_analysis'] for m in legs),
            ambiguous_legs=sum(m['ambiguous'] for m in legs),
            unmatched_selected_jets=sum(j['matched_leg_id'] is None for j in selected))
if args.generator is not None and len({s['run']['schema'] for s in samples})>1:
    parser.error('--generator cannot describe a mixed native/Fortran input set')
report=dict(scope='Generator event analysis; no matched Fortran/native ratio validation',full_goal_validated=False,
 generator=None if args.generator is None else dict(path=str(args.generator.resolve()),sha256=hashlib.sha256(args.generator.read_bytes()).hexdigest()),
 selection=dict(algorithm='anti-kt',R=.4,min_pt_GeV=10,max_abs_eta=2,
 particles='all final visible particles; exclude neutrino PDG 12/14/16',
 soft_drop=SOFT_DROP,mass='FastJet signed invariant mass in GeV; negative values retained',
 order='full-phase-space clustering before jet cuts',
 geometry_matching=dict(max_delta_r=.3,coordinates='pseudorapidity and azimuth',assignment='hepyy_utils.match_hard_legs: maximize matches then minimize total distance',order='all positive-pT jets before analysis cuts',unmatched_jet_geometry=None),recoils='per-run keep_recoils flag; use projected_particles when projection is enabled, otherwise subtracted_particles when subtraction is enabled, otherwise raw hadronized_particles'),
 normalization='weighted jets per denominator record: attempted events for native runs, written events for imported Fortran; not cross-section normalized or directly comparable without a configuration/normalization audit',
 analysis_versions={'source_sha256':{name:hashlib.sha256(Path(__file__).with_name(name).read_bytes()).hexdigest()
 for name in ['analyze_native_events.py','jet_substructure.py','native_geometry_analysis.py']},'python':platform.python_version(),'hepyy':importlib.metadata.version('hepyy'),
 'cppyy':importlib.metadata.version('cppyy'),'fastjet':str(fj.gbl.fastjet.fastjet_version_string()),
 'matching_sha256':hashlib.sha256(Path(match_hard_legs.__code__.co_filename).read_bytes()).hexdigest(),
 'loader_sha256':hashlib.sha256(Path(require_fastjet.__code__.co_filename).read_bytes()).hexdigest()},
 samples=samples)
args.output.write_text(json.dumps(report,indent=2,allow_nan=False)+'\n')
for sample in report['samples']:
 print(sample['run']['mode'],sample['summary'], 'selected jets:',sample['selected_jets'])
