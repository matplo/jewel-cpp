# Identical-state native cascade replay

Recorded 2026-11-21, macOS arm64, AppleClang 21/GNU Fortran 16.2, Release.
This checkpoint fixes a demonstrated C++ boundary error. It does not close the
full-generator validation gate.

## Capturing and replaying inputs

The development runner accepts an optional **new** directory for replay inputs:

```sh
build-pythia8/jewel-run-partons medium.jsonl 20 2601001 medium replay-inputs
python3 tools/replay_native_cascade.py build/jewel-medium-probe \
  build/jewel-medium-reference replay-inputs replay-results \
  --attempts 1 6 10 14 18 --timeout 45
```

Each fixture captures the exact initialized hard event, frozen remnants,
synthetic process anchor, run settings, SplitMix seed and consumed-draw cursor.
It is written before cascade evolution, so failed events can also be replayed.
The NCASCADE probe protocol accepts variable-length input and resumes the RNG
at that cursor. Input IDs are source slots at this pre-cascade boundary; the
fixture does not represent an arbitrary compressed native event.

The reference uses the existing diagnostic scheduler around unchanged
MAKEBRANCH and its extracted callees. It checks finiteness after each completed
branch and has a branch-call limit. A process timeout bounds loops inside a
routine. A timeout or nonfinite event is explicitly not a successful comparison.
This is not a comparison of independent Pythia6 and Pythia8 hard production.

The replay regression checks four vacuum cases with a nonzero initial RNG cursor:
10,824 values pass, maximum absolute error 3.1531e-14, with exact draw counts.
[Protocol regression result](cascade-native-input.json).

## Boundary correction

At t=z=0, AVSCATCEN returns zero mass and momentum. For a massless projectile,
GETPNORAD1 then forms a transient 0/0 in its scattering upper bound. Despite
that intermediate NaN, it returns a finite result through either its energy
cutoff or GETSSCAT==0 branch: one below Q0/SCALEFACM; otherwise minus one for
a gluon and one for a quark. C++ previously threw before reaching that branch.
It now preserves the returned result without materializing the transient NaN.

[180 added reference cases](validation-virtuality.md) cover zero energy,
cutoff neighbours, origin, both light-cone boundaries and outside-cone points,
with quarks/antiquarks/gluons and two table configurations. All agree exactly.
No probability clipping, random-stream change or relaxed tolerance was used.
Scattering contracts, virtuality reference tests, initial-evolution reference
tests and the generated cascade baseline all pass after this correction.

## Native sample and remaining mismatches

The identical 20-attempt medium sample improves from 16 to 18 accepted events.
The two remaining failures are attempts 6 and 10. Guarded Fortran replay also
encounters nonfinite states for those inputs, although at different stopping
points: C++ rejects inside a routine, while the reference guard checks after
a completed branch. This does not establish the same underlying numerical cause.
All attempts and failed weights remain in the native output.
[Updated integrity and accounting audit](validation-native-partons-fixed.json).

| Attempt | Before correction | After correction versus Fortran |
|---|---|---|
| 14 | C++ zero-invariant exception; Fortran finite | Both 242 records, 93 stored centres, 724,325 total draws; seven scalar mismatches |
| 18 | C++ zero-invariant exception; Fortran finite | Both 268 records, 98 stored centres, 25,567 total draws; six scalar mismatches |

For the recovered events, all compared discrete fields agree exactly. Largest
absolute differences are 1.73195e-12 and 1.26661e-12 respectively. Several small
momentum/position components exceed the existing abs=2e-13, rel=2e-12 tolerance;
these remain **failed strict comparisons**, not passes. The native runner and
C++ probe agree exactly in active-particle momenta, record counts and draw counts
for both recovered events.

Attempt 1 already showed a larger trajectory divergence before this correction:
C++ had 294 records/496,060 draws versus Fortran's 334 records/396,577 draws.
That mismatch remains unresolved. The diagnostic selection is not a statistical
jet comparison and cannot support the requested 10% physics-validation claim.

[Before correction](native-replay-before.json) and
[after correction](native-replay-after.json) preserve headers, mismatch examples,
input/executable hashes, source provenance and guard identity. The existing
1,000-shower report and its unresolved mismatches remain separate evidence.
