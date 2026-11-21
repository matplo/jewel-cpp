# Preserve provisional splitting fractions through collision retries

Recorded 2026-11-21. A controlled differential sweep found a genuine early
rejection in the C++ port. Four reduced-energy DOKINEMATICS inputs returned
finite records in JEWEL 2.6.0 but were rejected by native collision finalization.

## Trigger and correction

The failing cases have incoming status 14, energy fraction 0.6, target mass
20 GeV, and requested splitting fraction -1. They cover all four kinematic
modes. The first provisional collision contains a nonfinite splitting angle.
The source then retries, removes provisional records, and resets z to zero
for status-14 input before completing the collision. The C++ single-collision
fraction check previously stopped the sequence before that retry could happen.

Full sequences now call a private candidate-finalization function. It permits
the provisional fraction to follow the source operations and retry logic.
The public `finish_collision` API retains its strict fraction contract.
`apply_collisions` still works on private event/recoil copies and validates the
complete candidate before publishing either one. No nonfinite event, clamp,
sentinel reinterpretation or changed random-number prescription is introduced.

The new transaction test checks that an unrecovered nonfinite angle throws
without publishing event nodes, colour changes or recoil storage. Random draws
are deliberately not rewound, consistent with the existing failure contract.

## Evidence

[Before/after report](validation-recoil-fraction-retries.json) includes source
and executable hashes, fixture hashes, headers and run audits. The bounded
128-case sweep covers four kinematic modes, statuses 2/14, four target masses,
fractions -1/0 and energy fractions 1/0.6. Each individual probe has a timeout.

| Classification | Before | After |
|---|---:|---:|
| Finite reference outcomes matching native active records | 64 | 68 |
| Native rejection where Fortran is finite | 4 | 0 |
| Native rejection with nonfinite Fortran output | 60 | 60 |

Finite comparisons require exact headers, including RNG counts, and active
record agreement at rel=2e-12, abs=2e-13. The 60 shared nonfinite outcomes are
explicitly failures, not 60 additional parity successes. Recoil-ledger rows
are not compared by this new targeted sweep; the pre-existing collision-step
and sequence suites cover those separately.

Six relevant CTests pass: public collision contracts, sequence transaction
contracts, collision-step and collision-sequence differential tests,
native-input cascade differential tests, and the new fraction sweep. The
native runner recoil-controls regression also passes.

```sh
ctest --test-dir build \
  -R '^(recoil-fraction-differential|collision-contracts|collision-sequence-contracts|collision-step-differential|collision-sequence-differential|cascade-native-input-differential)$' \
  --output-on-failure
```

## Event-level impact and limits

The 20-attempt hard-cut-5 rescattering sample still accepts 14 events. All 14
accepted event records are exactly unchanged. Attempts 7, 9, 12 and 19 now
reject with a nonfinite shower-record error at publication instead of the
premature fraction precondition. Their final RNG cursors are 33463, 7154,
16578 and 6422 respectively, matching guarded Fortran. Attempt 9 previously
stopped eight draws earlier. Failed events remain in the accounting.

This fixes a translation mismatch in recoverable provisional states. It does
not make the shared failing sample events finite, validate production
rescattering, or satisfy the full-generator feature and jet-comparison goal.
