# Angular-rejection repair validation

Updated 2026-11-21. Expanded angular-rejection repair checks pass; see
[arithmetic regression and event replay](validation-rejection-arithmetic.md).

REJECTSPLITTING is extracted unchanged from hash-verified JEWEL 2.6.0 and run
with the original GETMASS and selected Pythia6 kinematics. No C++ branch samples
or momentum values are supplied to the Fortran calculation. Driver inputs are
identical event records and finite random tapes.

All 1,153 cases pass, comparing 110,664 record scalars plus discard flags and random
draw counts. Discrete fields and draw counts match exactly, with a maximum of
54 draws. Maximum absolute floating difference is 2.8422e-14 and maximum relative
difference 2.22398e-15. Acceptance tolerances are relative 2e-12 / absolute 2e-13.
Platform: macOS arm64, AppleClang 21.0.0 and GNU Fortran 16.2.0, Release.

Cases cover parent energies 20, 100 and 10000 GeV; ANGORD=0–5, quark/antiquark/gluon siblings, both/one/neither sibling
requiring resampling, aligned and rotated parent momenta, and rejected-scattering
records whose descendants become inactive. One impossible-kinematics fixture
checks event discard and the reference's early return in the temporary frame.
For each successful fixture, independent checks verify total four-momentum
conservation and each daughter's mass shell. For boosted parents the shell check
allows 32 ULPs of parent energy squared (or the previous 5e-11 GeV² floor) to
account for cancellation; this does not change the direct reference tolerance.
State validation checks genealogy.

Record comparisons include status, PDG code, parent/daughter links, momentum,
record mass, auxiliary coordinates, production coordinates, decay/evolution
times, splitting fractions, angular limits and branch flags. Colour fields are
not touched by this routine. The repaired lifetime follows the source's original
mass-squared rescaling and uses the final retry flags for evolution-time updates.

The reference-only target links extracted PYANGL/PYROBO/PYP/GETPOSITION bodies;
unused charge and mass lookup paths fail immediately in the driver. No Pythia6
library is linked into the C++ implementation. Undefined zero transverse
rescaling and parent-axis inputs throw in C++; no parity is claimed for them.
The C++ retry guard throws after one million attempts instead of hanging.

[Detailed results and source/executable hashes](validation-rejection.json).
This validates sibling repair, not accepted branch construction, full shower
histories, Pythia8 hadronization or jet observables.
