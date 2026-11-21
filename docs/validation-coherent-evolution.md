# Full coherent-evolution validation

Recorded 2026-11-20, macOS arm64, Release builds, AppleClang 21 and GNU Fortran 16.2.
`evolve_coherent()` implements DOCOHSCATTERING orchestration. The reference runs
the complete unchanged routine (lines 4644–5072), extracted only after checking
the frozen JEWEL source hash. Both implementations use their own medium, rate,
transfer and collision calculations with identical input records and RNG tapes.

[Machine-readable results](validation-coherent-evolution.json) contain settings,
source identities, executable/input hashes, errors and excluded nonfinite cases.

## Results

- 131 cases executed; 124 have wholly finite reference outputs.
- 382,958 values compared; largest absolute difference: 2.755e-12.
- Random-draw counts, record counts, statuses, ancestry indices, colour indices,
  flags and retained-transfer counts agree exactly in the finite cases.
- 94 finite cases store recoils; the largest event contains 50 records and the
  longest replay consumes 4,963 random draws.
- Seven cases undo kicks after failed daughter reconstruction: 13 rollbacks in
  total, up to five in one call. Eight cases exercise transfer-resolution rejection.

The matrix covers quark, antiquark and gluon parents, gluon-to-quark pairs, both
gluon colour directions, both selected siblings, KINMODE 0–3, RECMODE 0–3,
subtraction on/off, NF=3/5, thermal longitudinal boost on/off, ANGORD=0/5,
rotated momenta, zero rate/duration, disabled coherence, shortened sibling
lifetimes, negative event weights, retained histories of zero/one/two transfers,
low-energy parents and daughter fractions near their kinematic boundary.

Every active event row, all 100 retained/scratch transfer slots, allocated recoil
storage, transfer sums and both 100-bin coherent histograms are compared.
The reference's unused storage starts zeroed (recoil ledger slots use matching
sentinels); undefined stale-memory behavior is outside the parity claim.

## Numerical acceptance and nonfinite cases

Event records and all other scalar values use relative tolerance 2e-12 or
absolute tolerance 2e-13. Stored transfer four-vectors and their accumulated
four-vector use a separately reported absolute tolerance of 2e-11 GeV.
They are formed by subtracting comparable boosted momenta after feedback through
multiple kicks. The original strict scalar tolerance exposed residual differences
only in those saved momentum components, at roughly 3e-12 GeV; event fields,
invariants, decisions and RNG counts still pass their original checks. This is
an explicit composite-calculation tolerance, not a change to the physics code.

Seven RECMODE=1 subtraction cases produce NaN dummy masses in Fortran.
C++ throws `nonphysical recoil dummy projection`. The test verifies that these
reference nonfinite fields occur only in the dummy-mass slots and that C++
rejects them for the expected reason. They are excluded from finite parity;
no agreement on subsequent random draws or partial state is claimed because
Fortran continues after producing the NaN while C++ stops. New or unmatched
failure categories fail the test instead of silently joining this exclusion.

## Preserved source behavior

- The resolution test retains the original (invariant,qx,qy) component ordering.
- Transfer retries share the routine's 100-retry counter; rejecting a resolved
  kick clears its stored transfer but retains the source's accumulated-sum effects.
- Failed daughter reconstruction removes only the latest collision pair. Colour
  allocation, recoil storage and incoming round-trip arithmetic remain changed.
- Daughter propagation advances by the last accepted trial's interval. Final
  positions use the coherent parent's birth time in the source formula, even
  when a shortened final interval ends before the last scattering time.
- Final splitting restores the original sibling decay times and branch metadata,
  assigns colours and status 21, and returns the selected new daughter.
- Time and transfer histograms use strict open bounds and the event weight.

Capacity exhaustion marks the C++ event discarded. Undefined projections and
nonphysical final resplitting throw; callers must restore or discard their event,
recoil, transfer and histogram state. The RNG tape is not rewound. A loop guard
throws after one million trials instead of hanging.

## Scope

This completes the coherent-evolution component, including its internal failed-kick
rollback. It does not implement the enclosing shower scheduler or its separate
coherence-rejection/reversion policy. Initial-state evolution now has
[its own full-routine validation](validation-initial-evolution.md). Complete shower
integration, Pythia8 adapters, custom exports and end-to-end jet/substructure
validation remain open under the full project completion gates.

## Regression checks

All nine targeted reference-enabled suites pass: full coherent evolution and its
contracts, initialization, coherent arithmetic and its contracts, collision
sequences and their contracts, final-state scattering, and shower-state contracts.
All 16 C++-only contract suites also pass. The production C++ library has no
Fortran dependency.
