# Rejected-splitting arithmetic correction

Recorded 2026-11-21 on macOS arm64, AppleClang 21 / GNU Fortran 16.2 Release.

`REJECTSPLITTING` rebuilds daughter momenta when angular ordering rejects a
previous splitting. Its C++ implementation had the same ungrouped energy
products as the earlier `MAKESPLITTING` correction. The Fortran source squares
energy before multiplying by the fraction. Optimized reference assembly also
confirms fused parent-mass and longitudinal-momentum square subtractions, with
separate rounded daughter-mass squares. C++ now preserves that order explicitly.
No global compiler setting, kinematic clamp or reference tolerance changed.

## Controlled reference tests

The expanded suite includes 1,153 cases / 110,664 record scalars at parent
energies 20, 100 and 10000 GeV, all six angular modes, species combinations,
rotated axes and rejected-scattering histories. Both engines receive identical
records and tapes.

- Before: 384 scalar mismatches; maximum absolute error 2.72034e-10.
- After: zero mismatches; maximum absolute error 2.84217e-14, maximum relative
  error 2.22398e-15. Discard decisions and random-draw counts agree exactly.

Acceptance remains abs=2e-13 / rel=2e-12. The independent mass-shell check
accounts for parent-scale floating-point cancellation, as documented in the
[component validation](validation-rejection.md).

Nine selected CTest checks pass (52.70 seconds): rejected and accepted
splitting, initial splitting, cascade contracts, generated baseline, collision
retry, native-input replay and both selected compression comparisons. The
separate native runner/replay contract also passes for all six angular modes.

## Preserved native-event replay

The initialized [event 9 fixture](angular-mode1-regression/9.input) now completes
in C++ as well as Fortran. Both produce 192 records, 64 scattering centres and
20090 random draws. Previously C++ stopped nonfinite after 1791 draws.
This is **not yet a passing strict replay**: 208 scalar values exceed tolerance,
with maximum absolute difference 1.13580e-8.

[Event 14](angular-mode1-regression/14.input) remains nonfinite in both engines.
C++ stops at draw 13795, Fortran at 13798; neither is counted as successful parity.
This correction does not resolve every cascade numerical discrepancy.

## Native medium smoke sample

With seed 2601001, centrality 0–10%, the JEWEL table-energy prescription,
longitudinal flow and angular mode 1, the rebuilt runner accepts 19/20 attempts.
Only event 14 fails. hepyy/cppyy full-phase-space anti-kt R=0.4 clustering selects
nine jets with pT ≥10 GeV and |eta|<2. All 20 attempts remain in normalization.
This is an integration check, not the ≥1000-jet generator validation gate.

[Before/after reports, replay results and sample provenance](validation-rejection-arithmetic.json).
