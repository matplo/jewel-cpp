"""Native/Fortran ratios from common hepyy analysis; physics parity stays explicit."""
import argparse
import hashlib
import json
import math
from pathlib import Path

MIN_JETS = 1000
TOLERANCE = 0.10


def compare(data, native_index, reference_index):
    native, reference = (data['samples'][i] for i in (native_index, reference_index))
    if native['run']['schema'] != 'jewel-hadron-development-v1':
        raise ValueError('numerator must be a native hadron sample')
    if reference['run']['schema'] != 'jewel-fortran-hepmc-analysis-v1':
        raise ValueError('denominator must be an imported Fortran sample')
    blockers = ['Matched physics configuration has not yet been established by a configuration audit.']
    scopes = [s.get('denominator_scope', s['run'].get('denominator_scope', 'unknown')) for s in (native, reference)]
    if scopes[0] != scopes[1] or 'unknown' in scopes:
        blockers.append('Event normalization denominators differ or are unknown: ' + ' versus '.join(scopes))
    if native['run']['mode'] != reference['run']['mode']:
        blockers.append('Medium/vacuum settings differ.')
    enough = all(s['selected_jets'] >= MIN_JETS for s in (native, reference))
    if not enough:
        blockers.append('Each sample must contain at least 1,000 selected jets.')
    observables = {}
    for name in sorted(set(native['distributions']) | set(reference['distributions'])):
        if name not in native['distributions'] or name not in reference['distributions']:
            raise ValueError('observable missing from one sample: ' + name)
        left, right = native['distributions'][name], reference['distributions'][name]
        if len(left) != len(right) or not left:
            raise ValueError('empty or mismatched bins: ' + name)
        rows = []
        for n, r in zip(left, right):
            if (n['low'], n['high']) != (r['low'], r['high']):
                raise ValueError('bin edges differ: ' + name)
            x, y = n['jets_per_attempt'], r['jets_per_attempt']
            if not all(math.isfinite(v) for v in [n['low'], n['high'], x, y]) or n['low'] >= n['high']:
                raise ValueError('invalid bin or yield')
            ratio = x / y if y > 0 and x >= 0 else None
            if ratio is not None and not math.isfinite(ratio):
                raise ValueError('nonfinite ratio')
            uncertainty = None
            errors = [n['event_standard_error'], r['event_standard_error']]
            if any(e is not None and (not math.isfinite(e) or e < 0) for e in errors):
                raise ValueError('invalid event standard error')
            if ratio is not None and all(e is not None for e in errors):
                # Independent event ensembles, delta-method uncertainty. This
                # form remains defined when the numerator yield is zero.
                uncertainty = math.hypot(errors[0] / y, ratio * errors[1] / y)
                if not math.isfinite(uncertainty):
                    raise ValueError('nonfinite ratio uncertainty')
            status = ('within-10-percent' if 1-TOLERANCE <= ratio <= 1+TOLERANCE else 'outside-10-percent') if ratio is not None else 'undefined'
            rows.append(dict(low=n['low'], high=n['high'], native=x, reference=y,
                             native_standard_error=errors[0], reference_standard_error=errors[1],
                             ratio=ratio, ratio_standard_error=uncertainty, status=status))
        # Empty/undefined bins cannot silently pass the requested ratio test.
        passed = enough and all(row['status'] == 'within-10-percent' for row in rows)
        observables[name] = dict(bins=rows, numerical_gate='pass' if passed else 'not-passed',
                                 undefined_bins=sum(row['ratio'] is None for row in rows))
    return dict(native_index=native_index, reference_index=reference_index,
                selected_jets=[native['selected_jets'], reference['selected_jets']],
                minimum_selected_jets=MIN_JETS, sample_size_gate=enough,
                ratio_tolerance=TOLERANCE, observables=observables,
                physics_gate='not-established', blockers=blockers,
                normalization='weighted jets per original denominator record; no rescaling or shape normalization',
                uncertainty='independent-event delta method; errors do not relax the central-ratio gate',
                samples=[{k:v for k,v in s.items() if k not in ('events','distributions')} for s in (native, reference)])


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('analysis',type=Path)
    parser.add_argument('output',type=Path)
    parser.add_argument('--pair',action='append',required=True,help='native:reference sample indices; repeat for settings')
    args=parser.parse_args()
    data=json.loads(args.analysis.read_text())
    pairs=[]
    for pair in args.pair:
        indices=tuple(map(int,pair.split(':')))
        if len(indices)!=2 or any(i<0 or i>=len(data['samples']) for i in indices):
            parser.error('pair must contain two valid nonnegative sample indices')
        pairs.append(compare(data,*indices))
    report=dict(schema='jewel-jet-comparison-v1',full_goal_validated=False,
                analysis_sha256=hashlib.sha256(args.analysis.read_bytes()).hexdigest(),
                selection=data['selection'],analysis_versions=data.get('analysis_versions'),comparisons=pairs)
    args.output.write_text(json.dumps(report,indent=2,allow_nan=False)+'\n')
    print(args.output)


if __name__=='__main__':
    main()
