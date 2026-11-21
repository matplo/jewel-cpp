"""Replay generated imported survivors through unchanged JEWEL preparation."""
import collections, hashlib, json, math, subprocess, sys
from pathlib import Path
reference, sample, fixtures, output = sys.argv[1:]
rows = [json.loads(line) for line in Path(sample).read_text().splitlines()]
header = rows[0]
events = [row for row in rows if row['type'] == 'event']
failures = [row for row in rows if row['type'] == 'failure']
assert sorted(row['attempt'] for row in events + failures) == list(range(1, header['attempts'] + 1))
assert rows[-1]['completed'] == len(events)
payload = Path(fixtures).read_text()
lines = iter(payload.splitlines()); conversions = 0; records = 0
for event in events:
    assert next(lines) == 'PREPARE'
    count, first, second, eta = map(float, next(lines).split())
    count = int(count); records += count
    assert len(event['values']) == count * 10
    for i in range(count):
        source = list(map(float, next(lines).split()))
        # A positive mass becoming zero measures actual MAKEMASSLESS calls.
        if source[6] > 0 and event['values'][10*i+5] == 0:
            conversions += 1
assert next(lines, None) is None
run = subprocess.run([reference], input=payload, text=True, capture_output=True, check=True, timeout=120)
expected = [float(value) for value in run.stdout.split()]
actual = [value for event in events for value in event['values']]
assert len(actual) == len(expected) == records * 10
maximum = 0; mismatches = []
for i, (x, y) in enumerate(zip(actual, expected)):
    maximum = max(maximum, abs(x-y))
    okay = x == y if i % 10 == 0 else math.isfinite(x) and math.isfinite(y) and math.isclose(x, y, rel_tol=2e-12, abs_tol=2e-13)
    if not okay: mismatches.append(dict(value=i, native=x, reference=y))
report = dict(scope='Generated native survivor preparation; not complete cascade or Pythia6 event equivalence',
    seed=header['seed'], attempts=header['attempts'], prepared_events=len(events), records=records,
    values=len(actual), massless_conversions=conversions, max_absolute_error=maximum,
    statuses=dict(collections.Counter(int(x) for x in actual[::10])), failures=failures,
    mismatches=mismatches, relative_tolerance=2e-12, absolute_tolerance=2e-13,
    hashes={str(p): hashlib.sha256(Path(p).read_bytes()).hexdigest() for p in [reference, sample, fixtures, __file__]})
Path(output).write_text(json.dumps(report, indent=2)+'\n')
print(json.dumps({k:v for k,v in report.items() if k not in ['hashes','failures','mismatches']}, indent=2))
print('Failures by stage:', dict(collections.Counter(row['stage'] for row in failures)))
if mismatches: raise SystemExit(f'{len(mismatches)} reference mismatches')
