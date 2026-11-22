"""Native hadron/parton samples via hepyy/cppyy FastJet; not Fortran validation."""
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
parser.add_argument('--summary-only',action='store_true',help='Accumulate histograms without retaining individual events')
parser.add_argument('--generator',type=Path,help='Executable used to produce the supplied files')
parser.add_argument('--soft-drop-zcut', type=float, default=SOFT_DROP['zcut'],
                    help='Soft Drop momentum-sharing threshold for beta=0')
parser.add_argument('--atlas-particle-level', action='store_true',
                    help='Use |y| < 2.1 and exclude muons as well as neutrinos')
collection_options = parser.add_mutually_exclusive_group()
collection_options.add_argument('--exclude-thermal-residuals', action='store_true',
                                help='Cluster the subtracted collection without leftover thermal entries')
collection_options.add_argument('--raw-hadrons', action='store_true',
                                help='Cluster raw hadrons from the same events for a subtraction comparison')
args=parser.parse_args()
max_abs_jet_coordinate = 2.1 if args.atlas_particle_level else 2.
jet_acceptance = (dict(max_abs_eta=None, max_abs_rapidity=max_abs_jet_coordinate)
                  if args.atlas_particle_level else dict(max_abs_eta=2))
excluded_pdgs = (12, 13, 14, 16) if args.atlas_particle_level else (12, 14, 16)
if not 0 < args.soft_drop_zcut < .5:
    parser.error('--soft-drop-zcut must be between 0 and 0.5')
soft_drop_settings = dict(SOFT_DROP, zcut=args.soft_drop_zcut)
from hepyy_utils.fastjet import require_fastjet
fj=require_fastjet()
bins={'mass':[-50,0,1,2,4,6,8,12,20,40,80],
      'groomed_mass':[-50,0,1,2,4,6,8,12,20,40,80],
      'zg':[.1,.15,.2,.25,.3,.35,.4,.45,.5000000000000001],
      'pt':[10,15,20,30,40,60,80,100,150,250],
      'girth':[0,.02,.04,.06,.08,.1,.15,.2,.3,.4],
      'ptd':[0,.2,.3,.4,.5,.6,.7,.8,.9,1.0000001]}

bins['zg'] = [args.soft_drop_zcut, *[edge for edge in bins['zg'] if edge > args.soft_drop_zcut]]

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
    attempts=accepted_count=selected=excluded_thermal_residuals=0
    collection=None
    total_weight=0.
    moments={k:[[0.,0.] for _ in e[1:]] for k,e in bins.items()}
    counts={k:Counter() for k in bins};weighted={k:Counter() for k in bins}
    with path.open('rb') as source:
        for raw in source:
            digest.update(raw);record=json.loads(raw)
            if record['type']=='run':
                assert header is None
                header=record
                assert header['schema'] in ['jewel-parton-development-v1','jewel-hadron-development-v1','jewel-fortran-hepmc-analysis-v1']
                schema=header['schema']
                subtraction=header.get('subtraction',{})
                if args.exclude_thermal_residuals and not subtraction:
                    raise ValueError('--exclude-thermal-residuals requires subtraction in the event file')
                if args.raw_hadrons and schema!='jewel-hadron-development-v1':
                    raise ValueError('--raw-hadrons requires native hadron events')
                if schema=='jewel-parton-development-v1':
                    collection='particles'
                elif schema=='jewel-fortran-hepmc-analysis-v1':
                    collection='final_particles'
                elif args.raw_hadrons or not subtraction:
                    collection='hadronized_particles'
                else:
                    collection='projected_particles' if subtraction.get('put_on_shell') else 'subtracted_particles'
            elif record['type']=='summary':
                assert summary is None
                summary=record
            else:
                assert header is not None and summary is None
                attempts+=1
                assert record['attempt']==attempts
                geometry_matches=None
                jets=[];accepted=record['type']=='event';weight=record.get('weight',0.)
                if accepted:
                    particles=fj.gbl.std.vector[fj.PseudoJet]()
                    for p in record[collection]:
                        if args.exclude_thermal_residuals and p['thermal_residual']:
                            excluded_thermal_residuals += 1
                            continue
                        final=p['status'] in [3,4,5] if header['schema']=='jewel-parton-development-v1' else p['final']
                        if not final or abs(p['pdg']) in excluded_pdgs:continue
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
                        if args.atlas_particle_level:
                            for match in geometry_matches:
                                if match['matched']:
                                    jet = all_jets[match['jet_id']]
                                    match['jet_passes_analysis'] = jet.pt() >= 10 and abs(jet.rap()) < max_abs_jet_coordinate
                    for jet_id,jet in enumerate(all_jets):
                        coordinate = jet.rap() if args.atlas_particle_level else jet.eta()
                        if jet.pt()<10 or abs(coordinate)>=max_abs_jet_coordinate:continue
                        constituents=list(jet.constituents());total=math.fsum(p.pt() for p in constituents)
                        if total<=0:continue
                        girth=math.fsum(p.pt()*math.hypot(p.eta()-jet.eta(),math.remainder(p.phi()-jet.phi(),2*math.pi)) for p in constituents)/total
                        ptd=math.sqrt(math.fsum(p.pt()**2 for p in constituents))/total
                        jets.append(dict(pt=jet.pt(),eta=jet.eta(),mass=jet.m(),girth=girth,ptd=ptd,
                                         **soft_drop(jet,fj,soft_drop_settings)))
                        if geometry_matches is not None:
                            jets[-1].update(jet_id=jet_id,jet_rank=jet_id+1,**geometry_by_jet.get(jet_id,
                                dict(matched_leg_id=None,matching_delta_r=None,matching_ambiguous=None,geometry=None)))
                else:
                    failures[record.get('stage','generation')+': '+record.get('reason','generation failure')]+=1
                histograms={key:histogram([j[key] for j in jets],edges) for key,edges in bins.items()}
                accepted_count+=accepted
                total_weight+=weight
                selected+=len(jets)
                for key,edges in bins.items():
                    for i,count in enumerate(histograms[key]):
                        value=weight*count
                        moments[key][i][0]+=value
                        moments[key][i][1]+=value*value
                    for jet in jets:
                        v=jet[key]
                        category='undefined' if v is None else 'underflow' if v<edges[0] else 'overflow' if v>=edges[-1] else 'in_range'
                        counts[key][category]+=1;weighted[key][category]+=weight
                if not args.summary_only:
                    events.append(dict(attempt=record['attempt'],accepted=accepted,weight=weight,pt_hat_GeV=record.get('pt_hat'),jets=jets,histograms=histograms))
                    if geometry_matches is not None:events[-1]['hard_leg_matches']=geometry_matches
    assert summary is not None and summary['attempts']==attempts==header['attempts_requested']
    assert summary['accepted']==accepted_count
    assert math.isclose(total_weight,summary['generator_weight_sum'],rel_tol=1e-12)
    distributions={};accounting={}
    for key,edges in bins.items():
        distributions[key]=[]
        for i,(total,squared) in enumerate(moments[key]):
            mean=total/attempts
            error=math.sqrt(max(0.,squared-total*mean)/(attempts-1)/attempts) if attempts>1 else None
            distributions[key].append(dict(low=edges[i],high=edges[i+1],weighted_count=total,
                jets_per_attempt=mean,event_standard_error=error))
        accounting[key]={category:dict(jets=counts[key][category],weighted_count=weighted[key][category])
                         for category in ['in_range','underflow','overflow','undefined']}
    return dict(particle_collection=collection, excluded_thermal_residuals=excluded_thermal_residuals, particle_level='parton' if header['schema']=='jewel-parton-development-v1' else 'hadron',denominator_scope=header.get('denominator_scope','attempted-events'),observable_accounting=accounting,input=str(path),input_sha256=digest.hexdigest(),run=header,summary=summary,
        selected_jets=selected,failure_reasons=dict(failures),distributions=distributions,events=events)

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
 selection=dict(algorithm='anti-kt',R=.4,min_pt_GeV=10,**jet_acceptance,
 exclude_thermal_residuals=args.exclude_thermal_residuals, raw_hadrons=args.raw_hadrons,
 particles='per-sample particle_level: native partons use status 3/4/5; hadron samples use final flag; '
           + ('excluded absolute PDG IDs: 12, 13, 14, 16' if args.atlas_particle_level else 'exclude neutrino PDG 12/14/16'),
 soft_drop=soft_drop_settings,mass='FastJet signed invariant mass in GeV; negative values retained',
 order='full-phase-space clustering before jet cuts',
 geometry_matching=dict(max_delta_r=.3,coordinates='pseudorapidity and azimuth',assignment='hepyy_utils.match_hard_legs: maximize matches then minimize total distance',order='all positive-pT jets before analysis cuts',unmatched_jet_geometry=None),recoils='per-run keep_recoils flag; use projected_particles when projection is enabled, otherwise subtracted_particles when subtraction is enabled, otherwise raw hadronized_particles'),
 normalization='weighted jets per denominator record: attempted events for native runs and ledger-backed Fortran; otherwise written Fortran events; not cross-section normalized or directly comparable without a configuration/normalization audit',
 analysis_versions={'source_sha256':{name:hashlib.sha256(Path(__file__).with_name(name).read_bytes()).hexdigest()
 for name in ['analyze_native_events.py','jet_substructure.py','native_geometry_analysis.py']},'python':platform.python_version(),'hepyy':importlib.metadata.version('hepyy'),
 'cppyy':importlib.metadata.version('cppyy'),'fastjet':str(fj.gbl.fastjet.fastjet_version_string()),
 'matching_sha256':hashlib.sha256(Path(match_hard_legs.__code__.co_filename).read_bytes()).hexdigest(),
 'loader_sha256':hashlib.sha256(Path(require_fastjet.__code__.co_filename).read_bytes()).hexdigest()},
 samples=samples)
args.output.write_text(json.dumps(report,indent=2,allow_nan=False)+'\n')
for sample in report['samples']:
 print(sample['run']['mode'],sample['summary'], 'selected jets:',sample['selected_jets'])
