"""Audit native development-runner accounting; not a physics parity test."""
import argparse
from collections import Counter
import hashlib
import json
import math
from pathlib import Path


def check(condition, message):
    if not condition:
        raise ValueError(message)


def finite(value):
    if isinstance(value, float):
        check(math.isfinite(value), 'nonfinite JSON number')
    elif isinstance(value, dict):
        for child in value.values():
            finite(child)
    elif isinstance(value, list):
        for child in value:
            finite(child)


def audit(path):
    records = [json.loads(line) for line in path.read_text().splitlines()]
    finite(records)
    header, summary = records[0], records[-1]
    check(header['type'] == 'run' and summary['type'] == 'summary', 'incomplete run')
    check(header['schema'] in ('jewel-parton-development-v1', 'jewel-hadron-development-v1'), 'unknown schema')
    outcomes = records[1:-1]
    check([r['attempt'] for r in outcomes] == list(range(1, header['attempts_requested'] + 1)),
          'missing, duplicate or unordered attempt')
    counts = Counter(r['type'] for r in outcomes)
    check(set(counts) <= {'event', 'generation_failure', 'evolution_failure'}, 'unknown outcome')
    for field, kind in [('accepted', 'event'), ('generation_failures', 'generation_failure'),
                        ('evolution_failures', 'evolution_failure')]:
        check(summary[field] == counts[kind], 'summary count mismatch: ' + field)
    check(summary['attempts'] == len(outcomes), 'attempt count mismatch')
    for field, kind in [('accepted_weight', 'event'), ('failed_weight', 'evolution_failure')]:
        weight = math.fsum(r['weight'] for r in outcomes if r['type'] == kind)
        check(math.isclose(summary[field], weight, rel_tol=1e-12, abs_tol=1e-12), 'weight mismatch')
    check(math.isclose(summary['accepted_weight'] + summary['failed_weight'],
                       summary['generator_weight_sum'], rel_tol=1e-12, abs_tol=1e-12), 'lost weight')
    final_count = 0
    hadron_count = 0
    max_hadronization_residual = 0.
    for event in outcomes:
        if event['type'] != 'event':
            continue
        particles = event['particles'] + event['archived']
        ids = {p['id'] for p in particles}
        check(len(ids) == len(particles), 'duplicate stable ID')
        for p in particles:
            for link in [p['parent'], p['coherent_partner'], p['collision_source'], *p['daughters']]:
                check(link == 0 or link in ids, 'dangling ancestry')
        check(all(p['status'] not in (1, 2) for p in event['particles']), 'unfinished event')
        check(len(event['initial_hard']) == 2, 'missing hard pair')
        mapped = dict(event['input_sources'])
        check(len(mapped) == len(event['input_sources']), 'duplicate input mapping')
        check(set(mapped) <= ids, 'missing imported record')
        for p in event['initial_hard']:
            check(p['id'] in mapped, 'hard role missing from source mapping')
        final_count += sum(p['status'] in (3, 4, 5) for p in event['particles'])
        if header['schema'] == 'jewel-hadron-development-v1':
            inputs, products = event['hadronization_input'], event['hadronized_particles']
            check([p['id'] for p in inputs] == event['hadronization_sources'], 'hadronization source order')
            check(set(event['hadronization_sources']) <= ids, 'unknown hadronization source')
            finals = [p for p in products if p['final']]
            check(finals and all(p['colour'] == p['anticolour'] == 0 for p in finals), 'coloured hadronization output')
            total = [math.fsum(p['momentum'][j] for p in inputs) for j in range(4)]
            output = [math.fsum(p['momentum'][j] for p in finals) for j in range(4)]
            residual = max(abs(a-b) for a,b in zip(total, output))
            check(residual <= 1e-8 * max(1., total[3]), 'hadronization momentum mismatch')
            max_hadronization_residual = max(max_hadronization_residual, residual)
            hadron_count += len(finals)

    check(final_count == summary['final_records'], 'final-record count mismatch')
    failures = Counter((r['stage'], r['reason']) for r in outcomes if r['type'] == 'evolution_failure')
    return dict(input=str(path), sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
                mode=header['mode'], summary=summary, hadronized_final_particles=hadron_count,
                max_hadronization_momentum_residual_GeV=max_hadronization_residual,
                failure_reasons=[dict(stage=s, reason=r, count=n) for (s, r), n in failures.items()],
                audit='passed; accounting and record integrity only')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('inputs', nargs='+', type=Path)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    report = dict(scope='Native reduced PPJJ smoke tests, not Fortran event parity',
                  samples=[audit(path) for path in args.inputs])
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    for sample in report['samples']:
        print(sample['mode'], sample['summary'], sample['audit'])
