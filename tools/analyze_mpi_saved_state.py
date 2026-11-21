"""Verify read-only PYMULT saved-state traces and reference event identity."""
import argparse,hashlib,json,math
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('directory',type=Path);p.add_argument('baseline_hepmc',type=Path);p.add_argument('source',type=Path);p.add_argument('output',type=Path)
a=p.parse_args()
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
initial=[];uses=[]
for line in (a.directory/'run.log').read_text().splitlines():
 t=line.split()
 if not t or t[0] not in ['MPI_SAVED_INIT','MPI_SAVED_USE']:continue
 mode=None
 if t[0]=='MPI_SAVED_USE':mode=int(t.pop(1))
 values=list(map(float,t[1:]));assert len(values)==25 and all(map(math.isfinite,values))
 row=dict(parp82=values[0],vint149=values[1],saved_normalization=values[2:5],sampling_bins=values[5:])
 if mode is None:initial.append(row)
 else:row['mode']=mode;uses.append(row)
assert len(initial)==4 and len(uses)==30
assert {m:sum(x['mode']==m for x in uses) for m in [4,5,6]}=={4:10,5:10,6:10}
assert all(x['saved_normalization']==initial[0]['saved_normalization'] and x['sampling_bins']==initial[0]['sampling_bins'] for x in uses)
assert initial[0]['parp82']==2 and all(x['parp82']==1.8 for x in uses)
assert all(math.isclose(x['vint149']/initial[0]['vint149'],.81,rel_tol=1e-14) for x in uses)
identity=sha(a.directory/'events.hepmc')==sha(a.baseline_hepmc);assert identity
report=dict(scope='first ten calls each to PYMULT modes 4,5,6; full 1000-attempt event-output identity',initializations=initial,production_samples=uses,all_sampled_saved_bins_and_normalizations_unchanged=True,screening_squared_ratio=.81,hepmc_byte_identical=identity,baseline_hepmc_sha256=sha(a.baseline_hepmc),source_sha256=sha(a.source),artifacts={name:sha(a.directory/name) for name in ['reference','pythia.f','instrumentation.patch','params.dat','medium.dat','run.log','events.hepmc']},limitations=['Only first ten calls of each production mode are traced; no all-call state invariant claimed.','This diagnoses source behavior; it does not establish a Pythia8 replacement or final-state jet parity.'])
a.output.write_text(json.dumps(report,indent=2)+'\n')
print('PASS: 30 saved-state samples; 20 bins and three normalization values retained; full reference HepMC byte-identical')
