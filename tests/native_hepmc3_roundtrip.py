"""Read native HepMC3 back through hepyy and verify exact final-state export."""
import hashlib,json,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from export_native_hepmc3 import collection,final_particles,scan
source,hepmc,output=map(Path,sys.argv[1:])
header,summary,digest=scan(source)
expected=[r for r in map(json.loads,source.read_text().splitlines()) if r['type']=='event']
import hepyy
hepyy.load('hepmc3')
import cppyy
cppyy.include('HepMC3/ReaderAscii.h')
H=cppyy.gbl.HepMC3;reader=H.ReaderAscii(str(hepmc));particles=0;geometry_events=0
for record in expected:
 event=H.GenEvent();reader.read_event(event)
 assert not reader.failed()
 assert event.event_number()==record['attempt'] and list(event.weights())==[record['weight']]
 assert event.momentum_unit()==H.Units.GEV and event.length_unit()==H.Units.MM
 info=event.run_info()
 assert json.loads(str(info.attribute_as_string('jewel_native_run')))==header
 assert json.loads(str(info.attribute_as_string('jewel_native_summary')))==summary
 assert str(info.attribute_as_string('jewel_native_sha256'))==digest
 actual=list(event.particles());wanted=final_particles(record,collection(header));assert len(actual)==len(wanted)
 for p,q in zip(actual,wanted):
  v=p.momentum();assert [v.px(),v.py(),v.pz(),v.e()]==q['momentum']
  assert p.pid()==q['pdg'] and p.status()==1 and p.generated_mass()==q['mass']
  assert str(p.attribute_as_string('jewel_native_particle_id'))==str(q['id'])
  particles+=1
 for name in ['geometry','initial_hard','impact_parameter_fm','centrality_fraction','event_lifetime_fm']:
  if name in record:assert json.loads(str(event.attribute_as_string('jewel_'+name)))==record[name]
 if 'geometry' in record:geometry_events+=1
end=H.GenEvent();reader.read_event(end);assert reader.failed();reader.close()
assert hepmc.read_text().rstrip().endswith('HepMC::Asciiv3-END_EVENT_LISTING')
report=dict(events=len(expected),attempts=summary['attempts'],particles=particles,
 geometry_events=geometry_events,collection=collection(header),exact_particle_weight_metadata_roundtrip=True,
 native_sha256=digest,hepmc_sha256=hashlib.sha256(hepmc.read_bytes()).hexdigest())
output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
