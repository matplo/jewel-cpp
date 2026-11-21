# Initial-state radiation table validation

Recorded 2026-11-20 (America/Los_Angeles), macOS arm64, AppleClang 21.0.0
and GNU Fortran 16.2.0, Release. All eight CTest suites pass.

The differential suite compares GETINSPLITI, GETINSUDAKOV, INSUDAINT,
GETINSUDARED and GETINSUDAFAST against unchanged stock JEWEL routines.
DERIV mode 6 is dispatched by the reference harness using the stock expression.
All 1,000 rows of all four Sudakov channels are compared for four configurations.

| Component | Compared scalars | Maximum absolute difference |
|---|---:|---:|
| table | 20000 | 0 |
| splitting | 180 | 0 |
| direct-sudakov | 100 | 0 |
| interpolation | 112 | 0 |
| ratio | 80 | 0 |

The suite includes NF=0/3/5, multiple Q0/Lambda settings, medium boost on/off,
EMAX up to 6,500 GeV, all five initial splitting channels, degenerate/reversed
integration ranges, lower lookup boundaries, and high-side table extrapolation.
Acceptance tolerances are relative 2e-12 and absolute 2e-13. Exact agreement on
this platform is not a promise of bitwise identity on all compilers.

During validation, ordinary sixth-power evaluation changed adaptive integration
paths. Matching the reference's cube-then-square evaluation restored exact
agreement without widening the acceptance threshold or changing integration
accuracy. This ordering is explicit in the implementation.

High-energy construction uses the stock Ei linear tail extrapolation explicitly.
This is reference compatibility, not a claim that the extrapolated table is an
accurate mathematical Ei approximation. The bounded Ei API remains available.
The unusual stock initial Sudakov grid starting at Q0 squared is preserved.

[Machine-readable results and hashes](validation-radiation.json).
These tests do not validate complete ISR histories, final-state showers, parton
transport, fragmentation or jets. Those remain separate implementation gates.
