# Public-hook ISR fractions improve recoil reconstruction

Recorded 2026-11-21, Pythia8 8.317. The adapter now captures exact beam fractions
around accepted ISR branches through public UserHooks. No modified Pythia
library is required. The isolated source-observation build is used only for
validation. Composition still has failed comparisons; this is not a completed
MPI timing or evolution implementation.

## Capturing accepted branches

`doVetoISREmission` observes the pre-update beam slots and the candidate
mother/recoiler/emission IDs. At the next ISR callback, or at the final
parton-level callback, the preceding accepted branch's updated fractions are
available. Pending candidates whose emission record was rolled back are
excluded. Completed observations are filtered against the final parton record.
The observer never vetoes, alters particles or draws random numbers.

Snapshots now export `isr_beam_fractions`, including subsystem and particle IDs,
before/after A/B fractions, and the collision energy squared. Validation requires
exactly one observation per emission and exactly one advancing beam fraction;
missing, duplicated, nonfinite and misassigned records fail explicitly.

Across seeds 2600417 and 2600418, 2,000 ISR/MPI-on events contain 26,962 branches.
All **107,848 captured fraction values equal the actual source values exactly**.
The subsystem mass² computed as xA*xB*s also equals the source value exactly for
every branch. Every pre-existing generation field is unchanged, including
momenta, weights and statistics. Derived ISR recoil matrices/errors are the
intended changed fields and are excluded from that identity check.

## Reconstruction changes

The subsystem ISR mapper uses those fractions directly for its longitudinal
boost ratios, subsystem invariant mass and z. This avoids estimating canonical
beam kinematics from historical vectors that can drift slightly off mass shell
after boosted source operations. Incoming masses, transverse directions and
beam directions are still checked. The original single-system API is unchanged.

For massless sisters and light flavours (|PDG| <= 3), it uses the source
prescription Q² = pT²/(1-z), recovering pT from the recorded evolution scale.
Heavy-threshold branches have a different mass-subtraction prescription and
continue to use the independently checked spacelike reconstruction. They are
not silently treated as light-flavour branches. The azimuth is normalized to
[0,2pi), matching the source's sampling convention.

The reconstructed z differs from the source by at most 1.12e-16; exact beam
fractions do not make the inverse floating-point division exact. Maximum Q²
residual is 8.74e-11 GeV² for light/massless branches and 4.59e-10 GeV² for the
remaining massive reconstruction. These are observed errors, not universal
bounds.

## Remaining acceptance failures

The two 1,000-event samples retain the same pp 5020 GeV, pThat 20–100 GeV,
ISR/MPI-on settings used in the preceding audits.

| Seed | Events with complete individual maps | Rejected events | Failed composed partons | Affected mapped events |
|---|---:|---:|---:|---:|
| 2600417 | 999 / 1000 | 1 | 19 | 15 |
| 2600418 | 998 / 1000 | 2 | 12 | 7 |

This reduces individual rejections from 12 to 3 and failed composed partons
from 107 to 31. Maximum composed error drops from 2.5281e-5 to 1.0113e-6 GeV.
All raw events remain present. The three rejected attempts are seed 2600417
attempt 273, and seed 2600418 attempts 458 and 613.

The fixed copy gate remains 2e-9 GeV + 2e-11 |p|; the fixed composition gate
remains 1e-8 GeV + 1e-10 |p|. The composition audit writes failures and exits
nonzero. No numerical acceptance tolerance was widened. The separate
[source-matrix audit](validation-isr-source-trace.md) still establishes a
finite-precision discrepancy even when the actual source ISR matrices are used;
that does not excuse the additional reconstruction errors here.

[Detailed evidence, source-fraction comparisons and remaining failures](validation-isr-fractions.json).

Four native suites pass: input contracts, CLI, shower boundary and shower output.
Contracts include missing/duplicate observations and a nonadvancing fraction.
The CLI continues to verify that failed mappings retain their event records.
These tests do not supersede the failed large-sample composition gate.

Next work must address the remaining reconstruction differences and establish
an explicit source-based numerical budget for composed transport. Physical time
assignment, MPI evolution and final jet comparisons remain open. No Fortran
physics implementation or reference tolerance changed.
