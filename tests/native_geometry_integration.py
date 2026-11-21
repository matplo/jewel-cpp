"""Audit an analyzed native sample against the pre-association analysis."""
import copy,hashlib,json,sys
from pathlib import Path
new_path,old_path,output=map(Path,sys.argv[1:])
new=json.loads(new_path.read_text());old=json.loads(old_path.read_text())
assert len(new['samples'])==len(old['samples'])
reports=[]
for sample,prior in zip(new['samples'],old['samples']):
    stripped=copy.deepcopy(sample);stripped.pop('geometry_accounting',None)
    for event in stripped['events']:
        event.pop('hard_leg_matches',None)
        for jet in event['jets']:
            for field in ['jet_id','jet_rank','matched_leg_id','matching_delta_r','matching_ambiguous','geometry']:
                jet.pop(field,None)
    assert stripped==prior,'association changed existing analysis results'
    rows=[]
    for event in sample['events']:
        if not event['accepted']:continue
        matches=event['hard_leg_matches'];matched=[m['jet_id'] for m in matches if m['matched']]
        assert len(matched)==len(set(matched))
        selected_ids={j['jet_id'] for j in event['jets']}
        for match in matches:
            assert match['jet_passes_analysis']==(match['jet_id'] in selected_ids)
        for jet in event['jets']:
            match=next((m for m in matches if m['jet_id']==jet['jet_id']),None)
            if match is None:
                assert jet['matched_leg_id'] is None and jet['geometry'] is None
                assert jet['matching_delta_r'] is None
            else:
                assert jet['matched_leg_id']==match['leg_id'] and jet['geometry']==match['geometry']
                assert jet['matching_delta_r']==match['delta_r']<=.3
            rows.append(dict(attempt=event['attempt'],**jet))
    reports.append(dict(input_sha256=sample['input_sha256'],summary=sample['summary'],
        selected_jets=sample['selected_jets'],geometry_accounting=sample['geometry_accounting'],selected_jet_records=rows))
report=dict(scope='native geometry association, not matched Fortran/native physics validation',
 existing_analysis_unchanged=True,analysis_versions=new['analysis_versions'],selection=new['selection'],reports=reports,
 input_hashes={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in [new_path,old_path]})
output.write_text(json.dumps(report,indent=2,allow_nan=False)+'\n')
print('Original observables unchanged; all selected-jet and leg associations consistent')
