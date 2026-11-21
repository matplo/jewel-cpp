# Full initial-state scattering validation

Recorded 2026-11-20, macOS arm64, Release builds, AppleClang 21 and GNU Fortran 16.2.
`InitialScatteringEvolution::sample()` translates DOINSTATESCAT (lines 4132–4638),
including nested coherent evolution and its radiation-rejection reversion.
It samples the initial-state emission; its caller subsequently applies
MAKEINSPLIT and the resolved collision sequence.

[Recorded results](validation-initial-evolution.json) include settings, input and
executable hashes, source identities, errors and control-flow diagnostics.

## Results

All **52 finite reference cases** pass, comparing **140,556 values**. The largest
absolute difference is **7.70e-14**. Every scalar uses relative tolerance 2e-12 or
absolute tolerance 2e-13; random-draw counts and discrete fields must agree exactly.
The longest replay consumes 41,779 random draws. Events reach 16 records, and
11 cases retain recoil storage from coherent evolution.

The matrix exercises zero/nonzero rates and intervals, quark/antiquark/gluon
legs, selected sibling ordering, radiation and flavour changes, forced radiation,
RETRYSPLIT on/off, coherent evolution on/off, unresolved-transfer rejection,
KINMODE 0–3, retained input transfers, rotated momenta, NF=3/5, longitudinal
thermal boost on/off, and ANGORD=0/5. The source's trial loop reaches 353 attempts.
There are 20 unresolved-transfer rejections and 659 coherent reversions across
16 cases. The 10,000-trial fallback is implemented but not reached by this matrix.

Compared outputs include returned fraction, virtuality, flavour, duration and
flags; all active event fields including evolution times; transfer records/sums;
allocated recoil storage; and weighted coherent histograms. Coverage assertions
require actual radiation, flavour changes, unresolved rejection and coherent
reversion, rather than allowing the suite to pass with only no-scattering cases.

The complete reference-enabled regression suite passes **42/42 tests** (257.39 s).
The independent C++-only build passes **17/17 contract tests** (18.49 s).

## Reference compilation

The frozen source is SHA256-checked before extraction. GNU Fortran rejects a
second declaration of WEIGHT inside DOINSTATESCAT's same DOUBLE PRECISION list.
The extraction manifest records one exact declaration-only normalization:

`TAUEST,QSUMVECOLD(4),ZDUM,WEIGHT,` → `TAUEST,QSUMVECOLD(4),ZDUM,`

WEIGHT remains declared earlier in that list. The extractor requires an exact,
unique match. No executable statement, constant, control flow or routine call
is changed. This exception is explicit; the reference is not described as a
byte-identical extraction of this routine.

## Source behavior retained

- The first waiting time and lifetime draw are reused in their original retry
  scopes. Branching is sampled even when NORAD later forces X=1.
- The second GETDELTAT call still consumes random draws, after which stock
  unconditionally assigns NOSCAT=.TRUE. Its unreachable additional-scattering
  block is not activated in C++.
- A rejected radiation trial undoes the coherent event rows and restores the
  original siblings' statuses, links and decay times. Colour allocation, recoil
  storage, histograms and round-trip arithmetic are retained, as in Fortran.
- `snapshot_truncated()` preserves removed physical slots' evolution times.
  Constructors that leave STARTTIME unassigned inherit those saved values;
  explicit times, including final coherent daughters, take precedence. Stable
  particle IDs are never recycled. An ordinary full snapshot restores the entire
  saved state, including this metadata, rather than inheriting later changes.
- Coherent resolution retains the previously documented legacy component
  ordering. Probabilities are not silently clamped.
- The phase/formation checks and collapse to the first transfer retain the
  source's RNG order and returned timing, including RETRYSPLIT behavior when the
  first collision-time query reports no collision.

The reference driver zeros unused storage and uses matching recoil sentinels.
No claim is made for arbitrary uninitialized Fortran memory. Exceptions require
caller rollback/discard of event, transfer, recoil and histogram state; random
numbers are not rewound. Invalid propagation/virtuality domains and capacity
limits remain explicit C++ domain/discard policies.

## Remaining scope

This validates the initial-state scattering component. It does not assemble a
complete shower scheduler or provide Pythia8 import/hadronization. The custom
exports, complete-event generation, final jet/substructure comparisons and
interactive report remain required by the full completion gates.
