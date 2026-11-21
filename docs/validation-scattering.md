# Scattering-rate validation

Recorded 2026-11-20 (America/Los_Angeles), macOS arm64, AppleClang 21.0.0
and GNU Fortran 16.2.0, Release. All 14 reference-enabled CTest suites and
all six C++-only contract suites pass.

The C++ XSECINT, GETXSECINT and GETSSCAT implementations are compared against
unchanged stock routines. DERIV modes 11–14 are explicitly dispatched by the
reference harness using their original expressions. The validation compares
all four 1,000 × 100 grids, both axes and channel cross-section queries.

| Comparison | Scalars | Maximum absolute error |
|---|---:|---:|
| full-grids | 1203300 | 0 |
| integrals-and-cross-sections | 852 | 2.2204e-16 |

Three configurations cover NF=3/5, equal and different shower/scattering Lambda
values, boost off/on, SCALEFACM=1/1.2 and a Debye floor of 2 GeV that collapses
the mass grid to one value. Queries cover all QA/QB/GA/GB integrals, Q/G
incoming projectiles, Q/G/combined outgoing channels and local/minimum/maximum
medium modes. They include massive projectiles, positive/negative rapidity,
empty phase space, the origin with a massless projectile, and table extrapolation.

The constant-Debye table gets an explicit C++ bin-selection branch rather than
converting NaN from a zero grid width into an integer; the values agree with the
stock reference on this platform. The stock final interpolation already returns
the first interpolated value when its two mass coordinates coincide.

Full grids match exactly. The largest cross-section-query difference is at
floating-point rounding scale. Relative tolerance remains 2e-12 and absolute
tolerance 2e-13. The single-precision 2./3. colour factor is preserved.

[Detailed results, grid hashes and source provenance](validation-scattering.json).
These are cross-section and numerical-grid comparisons, not full transport or
jet-level validation. Collision scheduling, formation times, shower branch
construction, colour/recoil state and Pythia8 integration remain unfinished.
