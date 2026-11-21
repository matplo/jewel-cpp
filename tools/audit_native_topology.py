"""Audit raw experimental native-junction hadronization samples, streaming events."""
import argparse
from collections import Counter, defaultdict
import hashlib
import json
import math
from pathlib import Path

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('input', type=Path)
parser.add_argument('output', type=Path)
args = parser.parse_args()
failures = Counter()
attempts = accepted = junction_events = 0
accepted_weight = failed_weight = 0.0
maximum_residual = maximum_scaled_residual = 0.0
strict_component_failures = []
header = summary = None
digest = hashlib.sha256()
with args.input.open('rb') as stream:
    for line in stream:
        digest.update(line)
        event = json.loads(line)
        if event['type'] == 'run':
            assert header is None and attempts == 0
            header = event
            assert header['schema'] == 'jewel-hadron-development-v1'
            assert header['experimental_isr_mpi']
            continue
        assert header is not None and summary is None
        if event['type'] == 'summary':
            summary = event
            continue
        attempts += 1
        assert event['attempt'] == attempts
        if event['type'] != 'event':
            assert event['type'] in ['evolution_failure', 'generation_failure']
            failures[event.get('reason', event['type'])] += 1
            failed_weight += event.get('weight', 0.0)
            continue
        accepted += 1
        accepted_weight += event['weight']
        junction_events += bool(event['hadronization_junctions'])
        physical = [p for p in event['particles'] if p['status'] in [3, 4, 5]
                    and (header['keep_recoils'] or p['status'] != 3)]
        inputs = event['hadronization_input']
        assert event['hadronization_sources'] == [p['id'] for p in inputs]
        assert len(inputs) == len(physical)
        colours = defaultdict(lambda: [0, 0])
        for original, exported in zip(physical, inputs):
            for key in ['id', 'pdg', 'mass', 'colour', 'anticolour', 'momentum']:
                assert original[key] == exported[key], (attempts, key)
            assert exported['vertex_mm'] == [x * 1e-12 for x in original['production_fm']]
            for side, key in enumerate(['colour', 'anticolour']):
                if exported[key]:
                    colours[exported[key]][side] += 1
        for junction in event['hadronization_junctions']:
            assert junction['kind'] in [1, 2]
            for tag in junction['colours']:
                assert tag > 0
                colours[tag][1 if junction['kind'] == 1 else 0] += 1
        assert all(count == [1, 1] for count in colours.values()), attempts
        final = [p for p in event['hadronized_particles'] if p['final']]
        assert final and all(p['colour'] == p['anticolour'] == 0 for p in final)
        energy_scale = max(1.0, math.fsum(p['momentum'][3] for p in inputs))
        for component in range(4):
            before = math.fsum(p['momentum'][component] for p in inputs)
            after = math.fsum(p['momentum'][component] for p in final)
            maximum_residual = max(maximum_residual, abs(after - before))
            maximum_scaled_residual = max(maximum_scaled_residual, abs(after - before) / energy_scale)
            # Match the existing hadronizer boundary criterion; also retain the
            # stricter small-sample diagnostic instead of hiding cancellations.
            assert abs(after - before) <= 1e-8 * energy_scale, (attempts, component, before, after)
            if not math.isclose(before, after, rel_tol=1e-9, abs_tol=1e-7):
                strict_component_failures.append(dict(attempt=attempts, component=component, residual_gev=after-before))
assert summary is not None and summary['attempts'] == attempts
assert summary['accepted'] == accepted
assert summary['generation_failures'] + summary['evolution_failures'] == attempts - accepted
assert math.isclose(summary['accepted_weight'], accepted_weight, rel_tol=1e-12, abs_tol=1e-12)
assert math.isclose(summary['failed_weight'], failed_weight, rel_tol=1e-12, abs_tol=1e-12)
assert math.isclose(summary['generator_weight_sum'], accepted_weight + failed_weight, rel_tol=1e-12, abs_tol=1e-12)
report = dict(scope='Native junction boundary invariants; failed attempts retained, not generator equivalence',
              input=str(args.input), input_sha256=digest.hexdigest(),
              mpi_channel_policy=header.get('mpi_channel_policy', 'source-primary'),
              summary=summary, junction_events=junction_events, failures=dict(failures),
              max_momentum_component_residual_gev=maximum_residual,
              max_momentum_residual_over_total_energy=maximum_scaled_residual,
              acceptance='abs(component residual) <= 1e-8 * max(1 GeV, total input energy)',
              strict_component_diagnostic=dict(relative_tolerance=1e-9, absolute_tolerance_gev=1e-7,
                                               failures=strict_component_failures),
              boundary_invariants_pass=True)
args.output.write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps(report, indent=2))
