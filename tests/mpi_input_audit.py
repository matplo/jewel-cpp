"""Audit exported Pythia MPI subsystem coverage, independently of C++ validation."""
import hashlib
import json
import math
from pathlib import Path
import sys

reports = []
for name in sys.argv[2:]:
    path = Path(name)
    events = systems = emissions = 0
    maximum_pair_error = maximum_event_error = maximum_system_error = 0.0
    header = summary = None
    for line in path.open():
        event = json.loads(line)
        if event['type'] == 'run':
            header = event
            continue
        if event['type'] == 'summary':
            summary = event
            continue
        assert event['type'] == 'event', 'generation failure must be accounted for'
        events += 1
        assert event['multiple_parton_interactions']
        assert all(event[key] is None for key in ['isr_chains', 'isr_recoil_frames', 'primordial_recoil_frame'])
        particles = {p['id']: p for p in event['particles']}
        def bottom(index):
            seen = set()
            while not particles[index]['final']:
                assert index not in seen
                seen.add(index)
                daughters = particles[index]['daughters']
                assert len(daughters) == 1
                child = particles[daughters[0]]
                assert index in child['mothers'] and child['pdg'] == particles[index]['pdg']
                index = child['id']
            return index
        roots = {p['id'] for p in particles.values() if abs(p['status']) in (23,33)}
        radiation = {p['id'] for p in particles.values() if abs(p['status']) == 43}
        emissions += len(radiation)
        mapped_roots, mapped_final = set(), set()
        for number, system in enumerate(event['subsystems']):
            systems += 1
            assert system['index'] == number
            pair = system['process_outgoing']
            assert len(pair) == len(set(pair)) == 2
            assert not mapped_roots.intersection(pair)
            mapped_roots.update(pair)
            assert all(abs(particles[i]['status']) == (23 if number == 0 else 33) for i in pair)
            mothers = particles[pair[0]]['mothers']
            assert len(set(mothers)) == 2 and mothers == particles[pair[1]]['mothers']
            assert all(abs(particles[i]['status']) == (21 if number == 0 else 31) for i in mothers)
            scale = max(1., sum(abs(particles[i]['momentum'][3]) for i in pair + mothers))
            for k in range(4):
                residual = abs(math.fsum(particles[i]['momentum'][k] for i in mothers) - math.fsum(particles[i]['momentum'][k] for i in pair))
                maximum_pair_error = max(maximum_pair_error, residual)
                assert residual <= 1e-8 * scale
            final = system['outgoing_final']
            assert final == [bottom(i) for i in system['outgoing']]
            assert len(final) == len(set(final)) and not mapped_final.intersection(final)
            assert all(bottom(i) in final for i in pair)
            mapped_final.update(final)
            # Diagnostic only: Pythia primordial recoil can leave subsystem
            # closure residuals; do not silently enforce a root-level tolerance.
            for k in range(4):
                residual = abs(math.fsum(particles[i]['momentum'][k] for i in system['incoming']) - math.fsum(particles[i]['momentum'][k] for i in final))
                maximum_system_error = max(maximum_system_error, residual)
        assert mapped_roots == roots
        assert mapped_final == {bottom(i) for i in roots | radiation}
        beam_energy = math.fsum(p['momentum'][3] for p in particles.values() if abs(p['status']) == 12)
        for k in range(4):
            residual = abs(math.fsum(p['momentum'][k] for p in particles.values() if p['final']) - (beam_energy if k == 3 else 0))
            maximum_event_error = max(maximum_event_error, residual)
            assert residual <= 1e-8 * beam_energy
    assert summary and header and events == summary['returned'] == header['attempts_requested']
    reports.append(dict(path=str(path), sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
        seed=header['seed'], pythia_version=header['pythia_version'], events=events,
        subsystems=systems, secondary_pairs=systems-events, isr_emissions=emissions,
        max_root_pair_closure_error_gev=maximum_pair_error,
        max_whole_event_closure_error_gev=maximum_event_error,
        max_final_subsystem_closure_error_gev_diagnostic_only=maximum_system_error))
Path(sys.argv[1]).write_text(json.dumps(dict(scope='MPI input mapping only; no JEWEL evolution', reports=reports), indent=2)+'\n')
print(json.dumps(reports, indent=2))
