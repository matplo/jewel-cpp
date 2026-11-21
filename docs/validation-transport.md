# Collision-time sampling validation

All 19 reference-enabled tests and all eight C++-only contract tests pass.

GETDELTAT is extracted unchanged from hash-verified stock JEWEL 2.6.0 and
compared with `ScatteringRates::next_collision` using identical random tapes.
The reference uses the previously validated medium and cross-section routines;
no C++ density, bound or cross section is injected into the reference.

The 384 replay cases all pass. Of these, 101 accept a collision. Acceptance
flags and consumed random-draw counts match exactly (up to 37 draws per case).
The maximum absolute elapsed-time difference is 1.3323e-15 fm/c, with relative
acceptance tolerance 2e-12 and absolute tolerance 2e-13. Results were obtained
with AppleClang 21.0.0 and GNU Fortran 16.2.0 on macOS arm64, Release builds.

Coverage includes quark/gluon projectiles, elastic-only and combined outgoing
channels, longitudinal medium boost off/on, ordinary and constant Debye grids,
different production points, points outside the overlap, nonzero longitudinal
motion, early/late starts, short and zero intervals, lifetime truncation,
already-expired lifetime, zero and doubled cross-section multipliers.
C++ contracts also cover invalid energy/intervals and exhausted replay tapes.

The stock rate normalization factor 5, p/E propagation, six-empty-trial early
exit and lifetime semantics are retained. Bound violations are exposed as a
counter while preserving stock acceptance; this counter is diagnostic and is
not part of the replay comparison. The reference suppresses its repeated-line
warning only; it does not change the weight. Exhausting one million trials
throws in C++ rather than returning the stock incomplete-loop result.

The medium's documented zero-density extension outside the forward light cone
remains in force; reference parity is claimed only on defined reference paths.
These tests establish collision-time sampling parity, not collision application,
recoil construction, complete shower histories or final-state jets.

[Detailed results, inputs and source/executable hashes](validation-transport.json).
