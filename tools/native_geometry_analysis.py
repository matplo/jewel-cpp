"""Associate native hard-leg geometry with full-phase-space jet axes."""
import math
from hepyy_utils.jewel.matching import match_hard_legs

LENGTH_FIELDS=('L_geom_T_fm','L_hot_fm','L_hot_T_fm','int_T3_dl')


def associate_geometry(event, jets, max_delta_r=.3, min_pt=10., max_abs_eta=2.):
    """Jets must be all positive-pT inclusive jets, sorted by pT before cuts.

    Return leg matches (including jets outside analysis acceptance) and a jet-ID
    lookup. Only actually matched jets get a geometry estimate.
    """
    geometry=event['geometry'];hard=event['initial_hard']
    if len(geometry)!=2 or {g['leg_id'] for g in geometry}!={1,2}:
        raise ValueError('native geometry requires exactly two distinct hard legs')
    particles={p['id']:p for p in hard}
    if len(particles)!=len(hard) or len(hard)!=2:
        raise ValueError('native hard snapshot requires two distinct particles')
    if len({g['shower_start_id'] for g in geometry})!=2:
        raise ValueError('geometry legs share a shower-start particle')
    legs=[]
    for g in geometry:
        p=particles[g['shower_start_id']]
        if any(not math.isfinite(g[key]) or g[key]<0 for key in LENGTH_FIELDS):
            raise ValueError('invalid path-length estimate')
        if len(g['production_fm'])!=4 or not all(math.isfinite(v) for v in g['production_fm']):
            raise ValueError('invalid geometry production point')
        if len(p['momentum'])!=4 or not all(math.isfinite(v) for v in p['momentum']):
            raise ValueError('invalid shower-start momentum')
        legs.append(dict(leg_id=g['leg_id'],px=p['momentum'][0],py=p['momentum'][1],pz=p['momentum'][2]))
    # This call receives ALL jets. Analysis acceptance is attached only afterward.
    matches=match_hard_legs(legs,jets,max_delta_r=max_delta_r)
    axes={j['jet_id']:j for j in jets};by_leg={g['leg_id']:g for g in geometry};by_jet={}
    for match in matches:
        jet=axes.get(match['jet_id'])
        match['jet_passes_analysis']=jet is not None and jet['pt']>=min_pt and abs(jet['eta'])<max_abs_eta
        match['geometry']=by_leg[match['leg_id']]
        if match['matched']:
            by_jet[match['jet_id']]=dict(matched_leg_id=match['leg_id'],matching_delta_r=match['delta_r'],
                                       matching_ambiguous=match['ambiguous'],geometry=by_leg[match['leg_id']])
    return matches,by_jet
