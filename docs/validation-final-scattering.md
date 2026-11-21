# Final-state scattering sequence validation

DOFISTATESCAT is extracted unchanged from hash-verified JEWEL 2.6.0 and compared
with C++ using identical projectile records, initial transfers and random tapes.
Reference GETDELTAT, GETQVEC, GETNEWMASS and their numerical dependencies are
original routines; C++ results are not injected into the reference calculation.

| Comparison | Cases | Scalars | Maximum absolute difference |
|---|---:|---:|---:|
| Scattering/formation sequence | 120 | 196440 | 8.5266e-14 |
| 100-transfer cap | 2 | 3274 | 0 |

All 122 cases pass. Flags, active ranges, scatterer IDs and draw counts match
exactly. Maximum draw consumption is 2,150; the ordinary sequences retain up
to 22 active transfers and 33 cases return a mass different from the original.
Both capacity fixtures force an accepted collision at NEND=100 and consume
exactly three draws before the reference cap/reset behavior. Floating acceptance
remains relative 2e-12 / absolute 2e-13, without relaxed tolerances.
The complete reference-enabled suite passes 26/26 tests; the C++-only build
passes 10/10 contract tests.
Platform: macOS arm64, AppleClang 21.0.0 and GNU Fortran 16.2.0, Release.

Coverage combines NF=3/5, boost off/on, angular modes 0/5, quark/gluon projectiles,
mass 0/2 GeV, initial transfers below/above cutoff, zero/short/long available
times, formation factors 0.5/1/2 and cross-section factors 0/1/20. The complete
100-slot transfer array is checked, including inactive tail entries; this avoids
hiding changes after the source collapses the active range to its first transfer.
Returned mass, time, fraction/pair/threshold flags, active bounds, sum/invariant,
projectile and scratch records are also compared. Independent checks bound the
returned time/mass, active count and transfer invariant and verify restoration
of the projectile energy and record mass.

C++ contracts additionally cover missing initial transfers, invalid configuration,
aliased IDs, negative intervals and the no-collision/first-transfer reset path.
The reference's 101 attempts to repair an excessive accumulated transfer are
translated, but these fixtures do not claim to force exhaustion of that limit.
The larger GETQVEC retry limit has separate validation.

Whole-sequence replay exposed an earlier density rounding difference: stock
forms T**3 before multiplying by the species factor. C++ now uses explicit
integer-cube multiplication for local densities and bounds, replacing general
pow evaluation in bounds and a differently grouped local product. The tiny
bound difference had shifted collision times and then transfer kinematics.
No formulas, input settings, reference bodies or tolerances were retuned.

[Detailed results and source/executable/input hashes](validation-final-scattering.json).
This validates the final-state candidate sequence, not application of transfers
to an outgoing event, recoil bookkeeping, full coherent/initial-state evolution,
Pythia8 hadronization or final jet observables.
