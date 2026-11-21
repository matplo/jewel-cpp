# Coherent-scattering arithmetic validation

Recorded 2026-11-20, macOS arm64, Release builds, AppleClang 21 and GNU Fortran 16.2.
This validates sections of DOCOHSCATTERING, not its complete control flow.
[Results and provenance](validation-coherence.json) record the input/executable
hashes and the exact unchanged reference sections.

| Section | Cases | Defined reference cases | Compared finite values | Maximum absolute error |
|---|---:|---:|---:|---:|
| Resolution gate | 741 | 633 | 2,304 | 0 |
| Daughter reconstruction | 721 | 720 | 13,680 | 2.73e-12 |

Every compared value passes relative tolerance 2e-12 or absolute tolerance 2e-13.
Decision flags agree exactly. The gate accepts 279 and rejects 354 cases; the
momentum reconstruction accepts 204 and rejects 516 negative-transverse-square
cases. For accepted reconstructions the largest momentum-conservation residual
against the original input is 2.14e-14 GeV. Infinite zero-separation thresholds
are compared separately, including their sign.

The remaining 109 inputs have undefined Fortran arithmetic: 108 gate cases have
zero projectile three-momentum, and one reconstruction has zero axial momentum.
C++ throws domain errors for those inputs. They are excluded from finite-value
parity claims; their rejection classifications are checked and recorded.

Coverage includes rotated projectiles, massless/massive parents and daughters,
small and large energy fractions, impossible reconstructions, zero separation,
zero opening angle, and adjacent representable values around the strict gate
threshold. C++ contracts additionally check daughter mass shells, conservation,
invalid inputs and the original transfer-component convention.

The four targeted reference-enabled tests (coherence differential/contracts,
kinematics differential and shower-state contracts) pass. All 15 C++-only
contract suites also pass.

## Preserved source behavior

DOCOHSCATTERING uses ALLQS columns 1,2,3 in its resolution projection. Their
meaning is (transfer invariant, qx, qy), not (qx, qy, qz). This is retained
literally even though it is dimensionally surprising. Replacing it with the
physical three-vector would change the model and must be a separate decision.
The conversion constant retains default-REAL rounding of 0.2, and equality
with the resolution threshold does not reject a transfer.

Reconstruction rotates the coherent parent and auxiliary generator vertex into
its axis, solves for daughter 2 using the supplied energy fraction, and rotates
the accepted pair back. The transverse-square subtraction uses the fused
operation observed in the compiled reference. On negative transverse square,
the returned parent remains in the axial frame and no daughters are supplied,
matching the source attempt before its event rollback. The caller must perform
that rollback; this helper does not modify an event graph.

## Remaining integration

The coherent merge requires two incoming siblings to share one intermediate
record. That genealogy, collision loop, source-specific partial rollback,
position updates, final resplitting metadata/colours and histograms remain to
be connected and validated as full DOCOHSCATTERING calls. No complete shower,
Pythia8 event or final-jet validation is claimed here.
