"""Audit a mapped snapshot file against its prior unmapped file; write JSON evidence."""
import json,math,hashlib,sys
from pathlib import Path
from decimal import Decimal,localcontext
reports=[]
for path,prior in [(Path(sys.argv[1]),Path(sys.argv[2]))]:
 rows=[json.loads(x) for x in path.read_text().splitlines()];old=[json.loads(x) for x in prior.read_text().splitlines()]
 seed=rows[0]['seed']
 count=negative=massless=0;maxdiff=0;minq2=math.inf;maxroundoff=0
 assert len(rows)==len(old)
 for e,original in zip(rows,old):
  if e['type']!='event':assert e==original;continue
  p={a['id']:a for a in e['particles']};chains=e['isr_chains'];assert len(chains)==2
  seen=set()
  for chain in chains:
   assert chain['hard_incoming'] in e['hard_incoming'] and abs(p[chain['beam']]['status'])==12
   for b in chain['branches']:
    assert b['emitted'] not in seen;seen.add(b['emitted']);count+=1
    mother=p[b['mother']];emitted=p[b['emitted']];daughter=p[b['daughter']]
    q=[x-y for x,y in zip(mother['momentum'],emitted['momentum'])];assert q==b['spacelike_daughter']
    negative+=q[3]<0;minq2=min(minq2,b['virtuality_squared'])
    with localcontext() as ctx:
     ctx.prec=80;v=list(map(Decimal.from_float,q));exact=float(sum(x*x for x in v[:3])-v[3]*v[3])
    bound=32*math.ulp(max(1.,sum(x*x for x in q)))
    err=abs(exact-b['virtuality_squared']);assert err<=bound;maxroundoff=max(maxroundoff,err)
    if abs(mother['pdg']) in [1,2,3,21] and abs(daughter['pdg']) in [1,2,3,21] and emitted['mass']==0:
     z=daughter['momentum'][3]/mother['momentum'][3]
     assert 0<z<1
     predicted=emitted['scale']**2/(1-z);massless+=1;maxdiff=max(maxdiff,abs(predicted-b['virtuality_squared']))
  assert seen==set(e['isr_emissions'])
  stripped={k:v for k,v in e.items() if k!='isr_chains'};assert stripped==original
 reports.append(dict(seed=seed,events=sum(e['type']=='event' for e in rows),chains=sum(len(e.get('isr_chains',[])) for e in rows),branches=count,negative_energy_branches=negative,min_virtuality_squared=minq2,
  max_virtuality_roundoff_vs_decimal=maxroundoff,massless_evolution_formula_checks=massless,max_massless_evolution_formula_absolute_residual=maxdiff,
  prior_fields_exactly_unchanged=True,input=str(path),sha256=hashlib.sha256(path.read_bytes()).hexdigest()))
print(json.dumps(reports,indent=2))
Path(sys.argv[3]).write_text(json.dumps(reports,indent=2)+'\n')
