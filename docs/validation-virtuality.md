# Scattering virtuality and no-radiation validation

Updated 2026-11-21 with zero-invariant boundary coverage.

GETNEWMASS and GETPNORAD1 are extracted unchanged from hash-verified JEWEL
2.6.0. The reference computes its own Sudakov, splitting, PDF and scattering
tables from the same configuration; no C++ numerical results are injected.

| Comparison | Cases | Maximum absolute difference | Maximum draws |
|---|---:|---:|---:|
| Final-state virtuality update | 720 | 1.2435e-14 | 77 |
| Initial-state virtuality update | 720 | 1.4211e-14 | 16 |
| No-initial-radiation probability | 288 | 1.1103e-16 | 0 |
| Zero-invariant boundary probability | 180 | 0 | 0 |

Returned masses and branch fractions agree within tolerance; pair flags and
draw counts match exactly for virtuality updates. Probability differences are at rounding scale. Floating
acceptance uses relative tolerance 2e-12 and absolute tolerance 2e-13; flags and
random consumption require exact equality. Platform: macOS arm64, AppleClang
21.0.0 and GNU Fortran 16.2.0, Release.

Coverage includes NF=3/5, Q0=1/1.5, angular-ordering modes 0–5, quark/gluon
updates, initial/final-state sampling, increases/decreases/unchanged virtuality,
below-cutoff scales, insufficient energy, retained/resampled/removed branches
and the stock single-precision near-equal-scale threshold. Previous fractions
and pair flags start nonzero, exposing paths that preserve caller state.

Probability queries span quarks, antiquarks and gluons; massive/massless and
low/high-energy projectiles; central, displaced and cold positions; positive
and negative longitudinal coordinates; medium boost off/on; distinct shower
and scattering couplings; and non-unit SCALEFACM. The added boundary cases force the zero-total return
convention, including the gluon −1 sentinel. They cover t=z=0, both light-cone
boundaries, outside-cone points, zero energy and adjacent floating-point values
around Q0/SCALEFACM. GETPNORAD1 transiently computes 0/0 there, but its energy
cutoff or GETSSCAT==0 branch returns a finite result. C++ now returns that same
result directly instead of throwing or propagating NaN. No clipping or retuning is performed.

C++ contracts additionally check no-draw retained/cleared outputs, invalid
spacelike-input signs and the finite zero-invariant branch results. Retry loops
have a one-million-attempt guard. A vanishing Sudakov denominator throws rather
than returning an undefined stock ratio. Tables supplied to updated_mass must
share the sampler's run configuration; this is a caller responsibility.

[Detailed results, source/executable hashes and input fingerprints](validation-virtuality.json).
These checks do not validate momentum-transfer application, recoil construction,
coherent scattering, full event evolution or jet-observable agreement.
