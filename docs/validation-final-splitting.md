# Final-state splitting validation

Recorded 2026-11-20 (America/Los_Angeles), macOS arm64, AppleClang 21.0.0
and GNU Fortran 16.2.0, Release. All ten reference-enabled CTest suites and
all four standalone C++ contract suites pass.

The implementation translates SPLITFNCINT, GETSPLITI and the final-state branch
of GETSUDAKOV. The reference uses unchanged source bodies, with DERIV modes
1–4 dispatched by the harness using their stock expressions. Initial-state
GETSUDAKOV routing is not exercised by this test.

Both configurations use the complete 1,000 × 1,000 grid for each of QQ, GG, QG.
The reference and C++ dumps include all 3,000,000 values plus both grid axes.
Every double is compared, not just selected cells or aggregate summaries.

| Comparison | Scalars | Maximum absolute error |
|---|---:|---:|
| full-tables | 6004000 | 0 |
| lookups-and-sudakov | 816 | 0 |

Configurations use NF=3/5, Q0=1/1.5 GeV, Lambda=0.2/0.25 GeV, EMAX=50/200 GeV
and medium boost off/on. Queries exercise all four final-state channels,
ANGORD=0–5, zeta=1 and zeta=0.5, cutoff and near-cutoff scales, high-side table
extrapolation, and empty/reversed Sudakov intervals. In this routine ANGORD>0
shares a branch; full mode-specific shower behavior is not yet implemented.

The initial combined-gluon Sudakov mismatch was traced to contraction of the
NF*QG + GG expression. Preserving the stock intermediate rounding of QG
removed it. No acceptance tolerance was widened: relative 2e-12 and absolute
2e-13. All values now match exactly on this platform. Cross-platform results
still require validation.

C++ contracts reject invalid angular square roots, unsupported GQ final-state
queries, invalid scales/energies and invalid table construction settings.
The production table uses about 24 MB for the three grids. Native binary dumps
are test artifacts only, not a portable cache or event format.

[Full records, table hashes and source identities](validation-final-splitting.json).
This establishes numerical table/query parity; complete shower evolution,
scattering histories, hadronization and jet comparison remain outstanding.
