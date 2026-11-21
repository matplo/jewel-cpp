# Collision momentum reconstruction validation

`scatter_pair` translates DOKINEMATICS lines 5752–6135: one transfer's
projectile/scattering-centre momentum reconstruction, before colour assignment,
lifetimes, recoil storage and the enclosing sequence's target-mass retries.
The reference harness extracts that section unchanged, with the original
COMMON declarations and hash checks for both the source and section. The test
wrapper supplies the loop index and incoming state and ends at label 222.
This is not yet validation of complete DOKINEMATICS calls.

Of 384 cases, 381 have finite reference outputs. All 20,955 scalar comparisons
for those cases pass, with maximum absolute difference 3.56e-15. Flags, statuses,
flavours, colour-direction decisions and random counts match exactly. Numerical
acceptance remains relative 2e-12 / absolute 2e-13. The largest tape consumption
is 38 draws. There are 141 accepted centre-of-mass cases, 48 accepted
scatterer-rest-frame cases and 192 rejected/zero-transfer cases; 15 cases
sample a recoil eligible for splitting.

The matrix covers KINMODE 0–3, quark/gluon projectiles, masses 0, 2, -2 and
-8 GeV (negative record mass denotes spacelike virtuality), moving/rest thermal
centres, transfers 0 to -200 GeV², increasing/decreasing candidate mass,
last/intermediate transfer status handling and temperature-dependent recoil
mass. Both incoming and outgoing momenta, record masses, auxiliary vertices,
branching metadata, mass-change and remaining-transfer bookkeeping are compared.

## Reference numerical limitations

Three cases produce NaNs in Fortran: after a kinematic fallback sets the
recoil momentum to zero, recomputing E4²-m4² with fused arithmetic gives a tiny
negative result. C++ rejects these cases with a domain exception. They are
listed individually in the JSON and excluded from the finite-parity count;
they are not silently clamped, removed or advertised as finite agreement.
Random consumption up to the rejection agrees with the reference.

Two finite fallback cases have momentum-conservation residuals up to
1.17e-8 GeV, reproduced by C++. Their residual vectors are recorded for both
implementations. All accepted finite cases satisfy an independent 1e-6 GeV
conservation check, matching the scale of the source's diagnostic threshold.
The reference's literal `-1.-6` in the recoil mass-squared guard means -7,
not -1e-6; this port retains it rather than silently repairing the source.

Near-zero recoil masses amplified initial arithmetic differences. Explicit
fused evaluation now preserves the reference's transverse momentum and mass
cancellations. No reference statements or comparison tolerances were changed
to suppress those discrepancies. These results describe the tested Release
build with AppleClang 21 and GNU Fortran 16.2 on macOS arm64.

## Integration boundary

The function returns value-owned candidate records; it does not append them to
ShowerState, assign parent/daughter IDs or colours, create production/decay times,
store centres/dummies, or perform sequence rollback. It updates statuses before
returning a successful pair. The returned incoming/scratch momenta and auxiliary
vertices retain the reference's round-trip arithmetic. A rejected pair still
carries these updates and must not be treated as two outgoing particles.

The sampler must share Q0 and other run settings with the collision config.
The current single-transfer interface uses its incoming projectile as the
original recoil for temperature-dependent mass evaluation; chaining requires
passing the original sequence projectile separately during integration.

C++ contracts cover conservation, input immutability, zero/invalid transfers,
invalid mode and nonfinite configuration. The affected reference comparisons
(kinematics, recoil and collision) pass. The C++-only build passes all 12 contract tests.

[Results and provenance](validation-collision.json).
