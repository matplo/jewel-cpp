"""Audit candidate mapped ISR timing against captured snapshots and Fortran."""
import collections,hashlib,json,math,subprocess,sys
from pathlib import Path
cpp,reference,snapshot,sample,fixtures,output=sys.argv[1:]
rows=[json.loads(line) for line in Path(sample).open()]
header=rows[0];systems=[r for r in rows if r['type']=='system' and r['timing_error'] is None]
assert [s['fixture'] for s in systems]==list(range(len(systems)))
failures=[r for r in rows if r['type'] in ['mapping_failure','generation_failure'] or (r['type']=='system' and r['timing_error'])]
by_event=collections.defaultdict(list)
for row in systems:by_event[row['attempt']].append(row)
lines=iter(Path(fixtures).read_text().splitlines());inputs=[]
for system in systems:
 n,factor,x,y,lifetime,scatter,ntape=map(float,next(lines).split());n=int(n);ntape=int(ntape)
 records={i:list(map(float,next(lines).split())) for i in range(105,n+1)}
 tape=list(map(float,next(lines).split()));assert len(tape)==ntape
 assert (factor,x,y,lifetime,scatter)==(1,1.2,-.7,20,1) and all(u==.5 for u in tape)
 inputs.append((n,records))
assert next(lines,None) is None
assert {r['attempt'] for r in rows if 'attempt' in r}==set(range(1,header['attempts']+1))
assert rows[-1]['fixtures']==len(systems)
seen_fixtures=set();kinematic_values=0;kinematic_max=0;attachment_records=0;hard_pair_records=0;parent_links=0
for line in Path(snapshot).open():
 e=json.loads(line)
 if e['type']=='run':assert e['settings']==header['settings'];continue
 if e['type']!='event':continue
 p={v['id']:v for v in e['particles']}
 ordered_finals=list(reversed([v for v in e['particles'] if v['final']]))
 def staged(i):return [*p[i]['momentum'][:3],p[i]['energy_before_beam_repair']]
 for system in by_event.get(e['attempt'],[]):
  assert system['fixture'] not in seen_fixtures;seen_fixtures.add(system['fixture'])
  index=system['subsystem'];chains=e['subsystem_isr_chains'][index];frames=e['subsystem_isr_frames'][index];prim=e['primordial_subsystem_frames'][index]
  def transported(emitted,q):
   v=[q[3],*q[:3]]
   for frame in [*[f for f in frames if f['emitted']>emitted],prim]:
    m=frame['matrix_e_xyz'];v=[math.fsum(m[4*i+j]*v[j] for j in range(4)) for i in range(4)]
   return [*v[1:],v[0]]
  expected={};next_id=107;finals=[]
  for side,chain in enumerate(chains):
   branches=chain['branches']
   daughters=[transported(b['emitted'],b['spacelike_daughter'])+[-math.sqrt(b['virtuality_squared'])] for b in branches]
   expected[105+side]=[next_id if branches else 3+side]+(daughters[0] if branches else staged(chain['hard_incoming'])+[0])
   for j,branch in enumerate(branches):
    incoming=daughters[j+1] if j+1<len(branches) else p[chain['backward_lines'][-1]]['momentum']+[p[chain['backward_lines'][-1]]['mass']]
    expected[next_id]=[next_id+2 if j+1<len(branches) else 3+side]+incoming
    final=branch['emitted_final'];expected[next_id+1]=[next_id]+p[final]['momentum']+[p[final]['mass']]
    finals.append(final);next_id+=2
  native_for={v['id']:i for i,v in enumerate(ordered_finals,1+system.get('native_anchor_count',1))}
  if 'import_role_counts' in system:
   nsystems=len(e['subsystems']);nisr=len(e['isr_final'])
   assert system['import_role_counts']==[2,2*(nsystems-1),nisr,len(ordered_finals)-2*nsystems-nisr]
   assert system['native_weight']==e['weight']
  if 'incoming_native' in system:
   assert system['native_anchor_count']==2+4*len(e['subsystems'])
   assert system['incoming_native']==[3+4*index,4+4*index]
   assert system['hard_origins_native']==[5+4*index,6+4*index]
   assert system['hard_parents_native']==system['hard_origins_native'];parent_links+=2
   if 'hard_scheduling_parents_native' in system:
    assert system['hard_scheduling_parents_native']==(system['hard_origins_native'] if index==0 else [0,0])
   beam_ids=[c['beam'] for c in e['subsystem_isr_chains'][index]]
   owned={i for sub in e['subsystems'] for i in sub['outgoing_final']}
   remnants=[v for v in e['particles'] if v['final'] and v['id'] not in owned]
   assert all(v['mothers'] in [[beam_ids[0]],[beam_ids[1]]] for v in remnants)
   assert system['remnant_parent_counts']==[sum(v['mothers']==[beam] for v in remnants) for beam in beam_ids]
  if 'source_hard_pair' in system:
   def final_copy(root):
    seen=set()
    while not p[root]['final']:
     assert root not in seen;seen.add(root)
     assert len(p[root]['daughters'])==1
     root=p[root]['daughters'][0]
    return root
   pair=[final_copy(root) for root in e['subsystems'][index]['process_outgoing']]
   assert pair==system['source_hard_pair'];hard_pair_records+=len(pair)
   assert [native_for[root] for root in pair]==system['native_hard_pair']
  for emission in system['emissions']:
   if 'native_id' in emission:
    assert emission['native_id']==native_for[emission['final_id']]
    attachment_records+=1
   if 'native_parent' in emission:
    sides=[side for side,c in enumerate(chains) if any(b['emitted_final']==emission['final_id'] for b in c['branches'])]
    assert len(sides)==1 and emission['native_parent']==system['incoming_native'][sides[0]];parent_links+=1
  n,actual=inputs[system['fixture']];assert n==next_id-1 and [v['final_id'] for v in system['emissions']]==finals
  for i,values in expected.items():
   for col,(x,y) in enumerate(zip(actual[i][:6],values)):
    kinematic_values+=1;kinematic_max=max(kinematic_max,abs(x-y))
    assert x==y if col==0 else math.isfinite(x) and math.isclose(x,y,rel_tol=1e-10,abs_tol=1e-8),(e['attempt'],index,i,col,x,y)
assert seen_fixtures==set(range(len(systems))),'Snapshot audit did not cover every timing fixture'
payload=Path(fixtures).read_text();results=[]
for exe,args in [(cpp,['--emissions-only']),(reference,[])]:
 run=subprocess.run([exe,*args],input=payload,text=True,capture_output=True,check=True,timeout=120)
 results.append([float(x) for x in run.stdout.split()])
a,b=results;assert len(a)==len(b)
cursor=0;values=0;maximum=0;discarded_nonfinite=0
for system,(n,records) in zip(systems,inputs):
 assert a[cursor]==b[cursor]==system['draws']
 for row in range(105,n+1):
  base=cursor+1+5*(row-105)
  if row>=108 and row%2==0:
   expected=system['emissions'][(row-108)//2];expected=[*expected['production'],expected['end_time']]
   for x,y,z in zip(a[base:base+5],b[base:base+5],expected):
    values+=1;maximum=max(maximum,abs(x-y),abs(x-z))
    assert math.isfinite(x) and math.isfinite(y) and math.isclose(x,y,rel_tol=2e-12,abs_tol=2e-13) and math.isclose(x,z,rel_tol=2e-12,abs_tol=2e-13),(system['attempt'],row,x,y,z)
  else:
   assert all(x==0 for x in a[base:base+5]);discarded_nonfinite+=sum(not math.isfinite(v) for v in b[base:base+5])
 cursor+=1+5*(n-104)
assert cursor==len(a)
report=dict(scope='Candidate common-frame Pythia8 ISR timing; not validated native evolution or Pythia6 history equivalence',
 native_attachment_records=attachment_records,hard_pair_records=hard_pair_records,native_parent_links=parent_links,
 attempts=header['attempts'],systems_replayed=len(systems),emitted_values=values,max_timing_absolute_error=maximum,
 kinematic_values=kinematic_values,max_independent_kinematic_difference=kinematic_max,
 discarded_reference_nonfinite_values=discarded_nonfinite,failures=failures,
 negative_incoming_energies=sum(s['negative_incoming_energies'] for s in systems),
 maximum_emission_copy_error_gev=max(s['maximum_emission_copy_error_gev'] for s in systems),
 input_hashes={str(p):hashlib.sha256(Path(p).read_bytes()).hexdigest() for p in [snapshot,sample,fixtures]},
 executable_hashes={str(p):hashlib.sha256(Path(p).read_bytes()).hexdigest() for p in [cpp,reference]},
 audit_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
Path(output).write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({k:v for k,v in report.items() if k not in ['input_hashes','executable_hashes','failures']},indent=2));print('Recorded rejected events/systems:',len(failures))
