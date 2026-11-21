"""Export native hadron JSON to HepMC3 through hepyy's native C++ binding.

Final-state interchange only. Full ancestry and failed attempts remain in the
native file; run metadata preserves the attempted-event normalization ledger.
"""
import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import tempfile


def scan(path):
    header=summary=None;weights=[];accepted_weights=[];failed_weights=[];accepted=failures=generation=0;digest=hashlib.sha256()
    with path.open('rb') as source:
        for raw in source:
            digest.update(raw);record=json.loads(raw);kind=record['type']
            if kind=='run':
                if header is not None or weights:raise ValueError('duplicate or misplaced run header')
                if record['schema']!='jewel-hadron-development-v1':raise ValueError('native hadronized input required')
                header=record
            elif kind=='summary':
                if header is None or summary is not None:raise ValueError('misplaced summary')
                summary=record
            else:
                if header is None or summary is not None:raise ValueError('event outside run')
                if record['attempt']!=len(weights)+1:raise ValueError('attempt IDs must be contiguous and increasing')
                if kind not in ('event','evolution_failure','generation_failure'):raise ValueError('unknown event outcome')
                weight=record.get('weight',0.)
                if not math.isfinite(weight):raise ValueError('nonfinite event weight')
                if kind!='generation_failure' and 'weight' not in record:raise ValueError('missing generated-event weight')
                weights.append(weight)
                if kind=='event':
                    if 'weight' not in record:raise ValueError('missing accepted-event weight')
                    accepted+=1;accepted_weights.append(weight)
                    final_particles(record,collection(header))
                elif kind=='evolution_failure':failures+=1;failed_weights.append(weight)
                else:generation+=1
    if header is None or summary is None:raise ValueError('complete native run and summary required')
    if summary['attempts']!=len(weights) or header['attempts_requested']!=len(weights):raise ValueError('attempt count mismatch')
    if (summary['accepted'],summary['evolution_failures'],summary['generation_failures'])!=(accepted,failures,generation):raise ValueError('outcome count mismatch')
    if not math.isfinite(summary['generator_weight_sum']) or not math.isclose(math.fsum(weights),summary['generator_weight_sum'],rel_tol=1e-12,abs_tol=1e-12):raise ValueError('generator weight ledger mismatch')
    for field,values in [('accepted_weight',accepted_weights),('failed_weight',failed_weights)]:
        if not math.isfinite(summary[field]) or not math.isclose(math.fsum(values),summary[field],rel_tol=1e-12,abs_tol=1e-12):raise ValueError('outcome weight ledger mismatch')
    return header,summary,digest.hexdigest()


def collection(header):
    if header.get('subtraction',{}).get('put_on_shell',0):return 'projected_particles'
    if header.get('subtraction'):return 'subtracted_particles'
    return 'hadronized_particles'


def final_particles(record,key):
    particles=[p for p in record[key] if p['final']]
    if len({p['id'] for p in particles})!=len(particles):raise ValueError('duplicate final particle ID')
    for p in particles:
        if len(p['momentum'])!=4 or not all(math.isfinite(x) for x in p['momentum']):raise ValueError('invalid final momentum')
        if not math.isfinite(p['mass']):raise ValueError('invalid generated mass')
        if not isinstance(p['pdg'],int) or not p['pdg']:raise ValueError('invalid final PDG ID')
    return particles


def export(source,output):
    if output.exists():raise ValueError('output already exists')
    header,summary,digest=scan(source);key=collection(header)
    import hepyy
    hepyy.load('hepmc3')
    import cppyy
    for name in ['GenEvent','GenParticle','GenRunInfo','WriterAscii','Attribute']:
        cppyy.include('HepMC3/'+name+'.h')
    H=cppyy.gbl.HepMC3;shared=cppyy.gbl.std.make_shared
    run=shared[H.GenRunInfo]();run.set_weight_names(['nominal'])
    def attribute(obj,name,value):obj.add_attribute(name,shared[H.StringAttribute](value))
    attribute(run,'jewel_native_run',json.dumps(header,allow_nan=False))
    attribute(run,'jewel_native_summary',json.dumps(summary,allow_nan=False))
    attribute(run,'jewel_native_sha256',digest)
    attribute(run,'jewel_particle_collection',key)
    attribute(run,'jewel_record_scope','final particles only; no reconstructed ancestry, beams or physical production vertices')
    attribute(run,'jewel_normalization','unmodified event weights; denominator is all native attempts, including failures; see jewel_native_summary; no GenCrossSection assigned')
    temp=None;writer=None;written=0;second_digest=hashlib.sha256()
    try:
        with tempfile.NamedTemporaryFile(dir=output.parent,prefix=output.name+'.',delete=False) as stream:temp=Path(stream.name)
        writer=H.WriterAscii(str(temp),run);writer.set_precision(17)
        with source.open('rb') as stream:
            for raw in stream:
                second_digest.update(raw);record=json.loads(raw)
                if record['type']!='event':continue
                event=H.GenEvent(run,H.Units.GEV,H.Units.MM);event.set_event_number(record['attempt'])
                event.weights().clear();event.weights().push_back(record['weight'])
                for p in final_particles(record,key):
                    particle=shared[H.GenParticle](H.FourVector(*p['momentum']),p['pdg'],1)
                    particle.set_generated_mass(p['mass']);event.add_particle(particle)
                    attribute(particle,'jewel_native_particle_id',str(p['id']))
                for name in ['geometry','initial_hard','impact_parameter_fm','centrality_fraction','event_lifetime_fm']:
                    if name in record:attribute(event,'jewel_'+name,json.dumps(record[name],allow_nan=False))
                writer.write_event(event)
                if writer.failed():raise IOError('HepMC3 write failed')
                written+=1
        writer.close()
        if writer.failed():raise IOError('HepMC3 close failed')
        writer=None
        if written!=summary['accepted'] or second_digest.hexdigest()!=digest:raise ValueError('native source changed during export')
        # Atomic publication without replacing an existing destination.
        os.link(temp,output)
    finally:
        if writer is not None:writer.close()
        if temp is not None:temp.unlink(missing_ok=True)
    return dict(written_events=written,attempted_events=summary['attempts'],collection=key,input_sha256=digest)


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('input',type=Path);parser.add_argument('output',type=Path)
    args=parser.parse_args();print(json.dumps(export(args.input,args.output),indent=2))
