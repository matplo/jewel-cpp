"""Locate the first differing branch boundary in optional cascade diagnostics.

Inputs are stderr files from probes run with JEWEL_CASCADE_TRACE=1.
A matching prefix is not a proof that all event records agree within branches.
"""
import argparse
import hashlib
from difflib import SequenceMatcher
import json
import math
from pathlib import Path


def read(path):
    records = []
    for line in path.read_text().splitlines():
        if not line.startswith('CASCADE_TRACE '):
            continue
        fields = line.split()[1:]
        if len(fields) not in (6, 15):
            raise ValueError('malformed trace: ' + line)
        row = list(map(int, fields[:6])) + list(map(float, fields[6:]))
        if row[0] not in (0, 1) or len(row) != (15 if row[0] == 0 else 6):
            raise ValueError('invalid trace phase: ' + line)
        records.append(row)
    if not records:
        raise ValueError('no trace records in ' + str(path))
    return records


def compare(a, b):
    first_discrete = first_numeric = None
    maximum = 0.0
    for index, (x, y) in enumerate(zip(a, b)):
        if x[:6] != y[:6] and first_discrete is None:
            first_discrete = dict(index=index, cpp=x, fortran=y)
        if x[:3] != y[:3]:
            break  # Do not compare physical values from different branch visits.
        for column, (u, v) in enumerate(zip(x[6:], y[6:]), start=6):
            if math.isfinite(u) and math.isfinite(v):
                maximum = max(maximum, abs(u - v))
            if not (math.isfinite(u) and math.isfinite(v) and
                    math.isclose(u, v, rel_tol=2e-12, abs_tol=2e-13)):
                if first_numeric is None:
                    first_numeric = dict(index=index, column=column, cpp=x, fortran=y)
    # Align branch-entry state keys independently of call numbering. Extra
    # zero-draw finalization visits can shift numbering without shifting RNG.
    aa, bb = [r for r in a if r[0] == 0], [r for r in b if r[0] == 0]
    keys = lambda rows: [tuple(r[2:6]) for r in rows]
    alignment = SequenceMatcher(None, keys(aa), keys(bb), autojunk=False)
    shared = 0
    aligned_mismatches = []
    unmatched_a, unmatched_b = [], []
    for tag, i, j, k, l in alignment.get_opcodes():
        if tag != 'equal':
            unmatched_a.extend(aa[i:j]); unmatched_b.extend(bb[k:l])
            continue
        for x, y in zip(aa[i:j], bb[k:l]):
            shared += 1
            if any(not (math.isfinite(u) and math.isfinite(v) and
                        math.isclose(u, v, rel_tol=2e-12, abs_tol=2e-13))
                   for u, v in zip(x[6:], y[6:])):
                aligned_mismatches.append(dict(cpp=x, fortran=y))
    return dict(aligned_entries=shared, aligned_numeric_mismatches=aligned_mismatches,
                unmatched_cpp_entries=unmatched_a, unmatched_fortran_entries=unmatched_b,
                cpp_records=len(a), fortran_records=len(b),
                first_discrete_difference=first_discrete,
                first_numeric_difference=first_numeric, max_finite_absolute_error=maximum,
                cpp_last=a[-1], fortran_last=b[-1],
                unmatched_cpp=a[len(b):], unmatched_fortran=b[len(a):])


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('cpp', type=Path)
    parser.add_argument('reference', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    report = compare(read(args.cpp), read(args.reference))
    report['files'] = {str(p): hashlib.sha256(p.read_bytes()).hexdigest()
                       for p in [args.cpp, args.reference]}
    report['columns'] = ['phase (0 before, 1 after)', 'branch call', 'scheduling slot',
                         'record count', 'RNG cursor', 'discard', 'px', 'py', 'pz',
                         'energy', 'mass', 'fraction', 'evolution time', 'production time', 'decay time']
    report['relative_tolerance'], report['absolute_tolerance'] = 2e-12, 2e-13
    args.output.write_text(json.dumps(report, indent=2, allow_nan=False) + '\n')
    print(json.dumps(report, indent=2, allow_nan=False))
