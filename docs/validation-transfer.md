# Momentum-transfer validation

All 24 reference-enabled tests and all nine C++-only contract tests pass.

GETQVEC is extracted unchanged from hash-verified JEWEL 2.6.0. The reference
uses original GETSCATTERER, GETT, medium and Pythia6 kinematics bodies, with
identical input records and finite random tapes. No C++ scatterer or transfer
values are passed into the reference.

| Comparison | Cases | Record scalars | Maximum absolute difference |
|---|---:|---:|---:|
| Fresh transfer | 102 | 4386 | 2.2205e-16 |
| Fresh accumulated transfer | 102 | 4386 | 0 |
| Retry-limit fallback | 3 | 129 | 0 |
| Recalculated transfer | 27 | 1161 | 2.2205e-16 |

All 234 cases pass, covering 10,062 scalar values plus discard flags, scratch
status/PDG and exact draw counts. The maximum random consumption is 5,878 draws.
Floating tolerances remain relative 2e-12 / absolute 2e-13. Independent checks
verify exact restoration of projectile momentum/record mass, non-timelike sums
and consistency between stored transfer invariant and its four-vector.
Platform: macOS arm64, AppleClang 21.0.0 and GNU Fortran 16.2.0, Release.

Coverage includes NF=3/5, medium boost off/on, massless and massive projectiles
at 2/10/100 GeV momentum, full/reduced energy, positive and negative longitudinal
propagation, active and cold medium, initially zero/nonzero coherent sums and
recalculation from stored Fortran-generated centres. Those stored centres are
common inputs to both recalculation implementations, not C++ predictions.
Three null-sum fixtures force all 1,002 candidate attempts to fail and verify
the zero-transfer fallback, retained sum, scratch state and consumed draws.

Compared records include sampled t, kick four-vector, stored time, scatterer
species/momentum/mass/position, coherent sum and invariant, restored projectile,
auxiliary coordinates and scratch momentum/production point. C++ contracts
check capacity discard, aliased IDs, missing stored transfers, invalid fractions
and nonfinite accumulated state. Undefined zero axes/rest frames throw.

The negative-longitudinal high-energy fixture exposed a rounding difference in
the earlier thermal sampler: stock first assigns PMAX=10*TEMP, then multiplies
the power-transformed draw. C++ now preserves that grouping. A one-ulp scatterer
difference had been amplified by cancellation in transfer kinematics. No tests
or tolerances were weakened. The transfer rescaling also explicitly retains the
rounded mass square across MAX and subtraction, avoiding fused recomputation.

[Detailed results and source/executable/input hashes](validation-transfer.json).
This validates transfer sampling and accumulation, not complete coherent
scattering, LPM scheduling, recoil application or final jet observables.
