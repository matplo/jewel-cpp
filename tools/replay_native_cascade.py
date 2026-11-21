"""Compare identical initialized native hard states through both cascade probes.

The Fortran side uses bounded diagnostic scheduling around unchanged MAKEBRANCH.
Failures/timeouts are recorded, never counted as successful numerical agreement.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
import math
from pathlib import Path
import subprocess


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def execute(executable, fixture, directory, timeout):
    prefix = directory / (fixture.stem + '.' + executable.name)
    result = dict(executable=str(executable))
    with fixture.open('rb') as source, prefix.with_suffix(prefix.suffix + '.out').open('wb') as out, prefix.with_suffix(prefix.suffix + '.err').open('wb') as err:
        try:
            run = subprocess.run([str(executable)], stdin=source, stdout=out, stderr=err, timeout=timeout)
            result['returncode'] = run.returncode
        except subprocess.TimeoutExpired:
            result['timeout_seconds'] = timeout
    lines = prefix.with_suffix(prefix.suffix + '.out').read_text().splitlines()
    result['stderr'] = prefix.with_suffix(prefix.suffix + '.err').read_text()[-2000:]
    if result.get('returncode') == 0:
        result['header'] = list(map(int, lines[0].split()))
    return result, lines


def compare(fixture, args):
    a, al = execute(args.cpp, fixture, args.output_directory, args.timeout)
    b, bl = execute(args.reference, fixture, args.output_directory, args.timeout)
    report = dict(attempt=int(fixture.stem), fixture_sha256=digest(fixture), cpp=a, reference=b)
    if 'header' not in a or 'header' not in b:
        report['classification'] = 'incomplete execution'
        return report
    ah, bh = a['header'], b['header']
    if ah[5] or ah[6] or bh[5] or bh[6]:
        report['classification'] = 'failed or nonfinite cascade; not parity'
        return report
    if ah != bh:
        report['classification'] = 'header mismatch'
        return report
    assert len(al) == len(bl), 'output length mismatch'
    n = ah[0]
    # Only allocated/used recoil entries are physical comparison data.
    # Tail initialization is harness bookkeeping; histograms follow the full tail.
    capacity = 23000
    rows = list(range(n + 102 + ah[2])) + [n + 102 + capacity, n + 103 + capacity]
    values = mismatches = 0
    max_absolute = 0.
    examples = []
    for i in rows:
        ar, br = list(map(float, al[i].split())), list(map(float, bl[i].split()))
        assert len(ar) == len(br)
        for j, (x, y) in enumerate(zip(ar, br)):
            discrete = i == 0 or (1 <= i <= n and (j < 5 or 21 <= j <= 24 or j == 26)) or (n + 2 <= i <= n + 101 and j == 6) or (n + 102 <= i < n + 102 + ah[2] and j == 0)
            values += 1
            if math.isfinite(x) and math.isfinite(y):
                max_absolute = max(max_absolute, abs(x-y))
            if not (math.isfinite(x) and math.isfinite(y) and (x == y if discrete else math.isclose(x, y, rel_tol=2e-12, abs_tol=2e-13))):
                mismatches += 1
                if len(examples) < 10:
                    examples.append(dict(row=i, column=j, cpp=x, reference=y))
    report.update(classification='scalar mismatch' if mismatches else 'passed', values=values,
                  mismatches=mismatches, examples=examples, max_absolute_error=max_absolute)
    return report


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ['cpp', 'reference', 'fixtures', 'output_directory']:
        parser.add_argument(name, type=Path)
    parser.add_argument('--attempts', nargs='+', type=int)
    parser.add_argument('--timeout', type=float, default=60)
    args = parser.parse_args()
    args.cpp, args.reference = args.cpp.resolve(), args.reference.resolve()
    args.output_directory.mkdir(exist_ok=False)
    fixtures = sorted(args.fixtures.glob('*.input'), key=lambda p: int(p.stem))
    if args.attempts:
        fixtures = [p for p in fixtures if int(p.stem) in args.attempts]
        assert len(fixtures) == len(set(args.attempts)), 'missing requested fixture'
    assert fixtures, 'no fixtures'
    with ThreadPoolExecutor(max_workers=2) as pool:
        results = list(pool.map(lambda p: compare(p, args), fixtures))
    report = dict(scope='identical Pythia-initialized states; guarded Fortran scheduler',
                  executables={str(p): digest(p) for p in [args.cpp, args.reference]},
                  medium_backends=sorted({'vacuum' if p.read_text().startswith('VACUUM\n') else 'expanding' for p in fixtures}),
                  vacuum_source_sha256='c83fda6a0b81ce0dc9f524bbf9a3c68f0225f090107a5cd4d6711edad0b20ea0' if any(p.read_text().startswith('VACUUM\n') for p in fixtures) else None,
                  guard_sha256=digest(Path(__file__).parents[1]/'reference/guarded_cascade.f90'),
                  sources=json.loads((Path(__file__).parents[1]/'reference/medium-source.json').read_text()),
                  relative_tolerance=2e-12, absolute_tolerance=2e-13, results=results)
    (args.output_directory/'report.json').write_text(json.dumps(report, indent=2, allow_nan=False)+'\n')
    for r in results:
        print(r['attempt'], r['classification'], r['cpp'].get('header'), r['reference'].get('header'), flush=True)
