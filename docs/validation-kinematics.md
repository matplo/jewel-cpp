# Kinematics and shower-state validation

Recorded on macOS arm64 using AppleClang 21.0.0 and GNU Fortran 16.2.0,
Release builds. All 18 reference-enabled CTest suites and all eight C++-only
contract suites passed. This is component validation, not event or jet-spectrum parity.

| Comparison | Scalars | Maximum absolute error |
|---|---:|---:|
| Rotation and boost, including auxiliary coordinates and rescale flag | 3645 | 7.1055e-15 |
| Momentum magnitude, polar/azimuthal angles, rapidity and pseudorapidity | 1535 | 2.2205e-16 |
| GETPOSITION propagation | 903 | 0 |

All 6,083 comparisons pass at relative tolerance 2e-12 and absolute tolerance
2e-13. Cases include 300 deterministic random configurations, longitudinal and
zero momentum, tiny rotations and boosts, spacelike records, near-light-speed
boosts and boosts requiring the reference rescaling. The rounding of the
rescaled boost factor is retained explicitly; tolerances were not relaxed.
Agreement is specific to the recorded compiler configuration.

The reference extracts unchanged PYANGL, PYROBO and PYP from the modified
Pythia6 source supplied with JEWEL, and GETPOSITION from stock JEWEL 2.6.0.
Hashes and routine spans are checked before compilation. PYP coverage is
limited to fields 8, 13, 15, 17 and 19. PYROBO coverage uses one active particle.
The test driver counts the boost-rescale warning and supplies fail-fast stubs
for unused mass/charge lookups; it is not a complete Pythia6 reference runtime.
The standalone C++ implementation has no Pythia6 linkage.

Independent C++ contracts check boost invariants and inverse transforms,
propagation speed bounds, snapshot rollback without ID reuse, failed-restore
state preservation, inserted ISR ancestors and ancestry-cycle rejection.
Production coordinates and auxiliary generator coordinates have distinct fields.
These state checks establish data-structure contracts, not Fortran shower parity.
MAKESPLITTING, transport scheduling and full event construction remain open.

[Detailed results and source/executable hashes](validation-kinematics.json).
