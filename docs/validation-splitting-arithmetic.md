# Splitting arithmetic correction

Recorded 2026-11-21, AppleClang 21 / GNU Fortran 16.2, macOS arm64 Release.

`MAKESPLITTING` squares parent energy before multiplying by the splitting
fraction. C++ previously evaluated an ungrouped product of those factors.
Near cancellation, the different rounding affects daughter transverse momenta.
The corrected code preserves the energy-square grouping and uses explicit
`std::fma` for the parent-mass and longitudinal-momentum square subtractions.
Daughter-mass squares retain their separate rounding, as in the frozen reference.
No global compiler flag, clamp, rejection rule or reference tolerance changed.

The operation order was checked against the unchanged source and optimized
GNU Fortran assembly: energy square; multiplication by 2z; fused subtraction
of parent mass squared; separate subtraction/addition of daughter mass squares;
division by twice parent longitudinal momentum. The transverse square similarly
fuses subtraction of longitudinal momentum squared before subtracting daughter
mass squared. This is a numerical compatibility choice for the recorded reference
build, not a promise of bitwise equality across compilers.

## Regression evidence

The original 155 splitting cases pass with maximum absolute error 1.77636e-14
(previously 6.3949e-13). Adding 36 boosted/asymmetric cases gives 191 cases and
24,882 compared scalars:

- Previous implementation: 18 scalar mismatches, maximum absolute error
  1.19326e-9.
- Corrected implementation: no mismatches, maximum absolute error 2.27374e-13;
  all particle counts, colour allocation, discard flags and random-draw counts
  match exactly. Each scalar passes abs=2e-13 / rel=2e-12.

Seven selected CTest checks pass: splitting, rejection, cascade contracts,
generated baseline, collision retry, compression differential and medium
compression. The native-input cascade comparison passes separately, as does the native
runner/replay contract test for all six angular modes (66.25 seconds).

## Event-level limits

The same 20-attempt medium sample (seed 2601001, centrality 0–10%, JEWEL table
energy, longitudinal flow on, angular mode 1) accepts 18 events instead of 19.
The original event 9 still fails in C++ while Fortran completes. Event 14 now
also fails in C++; Fortran independently fails on this same initialized input:
C++ stops at draw 13795, Fortran at 13798. Both nonfinite outcomes are recorded
as failures, **not numerical parity**. The preserved [event 14 input](angular-mode1-regression/14.input)
allows further investigation. The additional failure is not evidence of improved
physical accuracy, and no failed attempt is silently removed from normalization.

hepyy/cppyy analysis of the new sample selects only eight jets (anti-kt R=0.4,
pT ≥10 GeV, |eta|<2, full-phase-space clustering first). This is a smoke check,
not the required ≥1000-jet Fortran/native comparison or a completed physics gate.

[Machine-readable before/after tests and replay evidence](validation-splitting-arithmetic.json).
