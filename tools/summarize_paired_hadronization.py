"""Summarize the common event ensemble before and after both hadronizers."""
import argparse
import json
import math
from pathlib import Path
import statistics

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('directory',type=Path)
p.add_argument('output',type=Path)
a=p.parse_args()
data=json.loads((a.directory/'jets.json').read_text())
samples=data['samples']
names=[s['run']['fragmentation_stage'] for s in samples]
assert names==['before','pythia6','pythia8']
groups=[s['events'] for s in samples]
assert len({len(g) for g in groups})==1
def stats(values):
    return dict(mean=statistics.mean(values),standard_error=statistics.stdev(values)/math.sqrt(len(values)))
def contrast(sums):
    return stats([y-x for x,y in zip(sums[1],sums[2])])
totals=[[e['weight']*len(e['jets']) for e in g] for g in groups]
bins={}
for key in ['mass','ptd']:
    rows=[]
    for i,b in enumerate(samples[0]['distributions'][key]):
        values=[[e['weight']*e['histograms'][key][i] for e in g] for g in groups]
        rows.append(dict(low=b['low'],high=b['high'],weighted_counts=dict(zip(names,map(sum,values))),
                         pythia8_minus_pythia6_per_event=contrast(values)))
    bins[key]=rows
result=dict(audit=json.loads((a.directory/'audit.json').read_text()),selection=data['selection'],
            input_sha256=dict(zip(names,[s['input_sha256'] for s in samples])),
            selected_jets=dict(zip(names,[s['selected_jets'] for s in samples])),
            pythia8_minus_pythia6_yield_per_event=contrast(totals),
            selected_jet_means={name:{k:statistics.mean(j[k] for e in g for j in e['jets'])
                                      for k in ['mass','ptd']} for name,g in zip(names,groups)},bins=bins)
a.output.write_text(json.dumps(result,indent=2)+'\n')
