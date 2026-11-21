# Updated vacuum jet comparison after native input recovery

Native commit 0f28348 completes all 1,500 attempts at seed 2600421. The previous
native sample completed 1,467. Hard/ISR/MPI settings and the Pythia cross-section
estimate remain unchanged; this update includes hard junctions with the first-
splitting veto off, inactive MPI photons, and inactive massive colourless
nonpartons under the documented finite unused-time convention.

The reference is the same frozen-source JEWEL 2.6.0 true-vacuum sample: 1,000
attempts, 997 written events and three recorded discards. Its full attempt ledger
is retained. Both samples use pp at 5020 GeV, CT14nlo member 0, hard pT 20–100 GeV,
ISR/MPI on, Q0=1 GeV, Lambda=0.2 GeV, NF=3, JEWEL EOVEST table scale and mapped
JEWEL fragmentation settings. Generator/tune equivalence remains unproven.

Common hepyy/cppyy analysis finds 2,126 native and 1,357 Fortran jets with anti-kt
R=0.4, pT>10 GeV and |eta|<2, after full-phase-space clustering. Neutrinos are
excluded. Soft Drop uses zcut=0.1, beta=0. All observables use weighted yields
per original attempt, with no shape renormalization or bin changes.

| Jet pT (GeV) | Native / Fortran | Ratio standard error |
|---|---:|---:|
| 10–15 | 1.140 | 0.074 |
| 15–20 | 0.924 | 0.057 |
| 20–30 | 1.017 | 0.058 |
| 30–40 | 1.113 | 0.124 |
| 40–60 | 1.290 | 0.259 |
| 60–80 | 1.111 | 0.452 |

The 10% gate still fails. Higher-pT bins are undefined; these do not count as
passes. Mass, girth, pTD, zg and groomed mass also have failing or undefined bins.
For example, girth 0.02–0.04 has ratio 1.407, and zg 0.45–0.5 has ratio 1.254.
Sampling uncertainty is displayed separately and does not relax the central-
ratio gate. Recovering the omitted events is insufficient to explain away the
generator differences. Larger samples and the remaining physics-configuration
audit are still needed; no parameters were tuned to these ratios.

[Interactive comparison](inclusive-vacuum-comparison.html) includes selectable
current and historical samples, all six observables, event-based errors, a
10% band, per-bin values and CSV export. The historical data are retained without
recomputation; each comparison carries its original analysis hash. The current
configuration audit records the executable hash and source commit.
[Comparison JSON](inclusive-vacuum-comparison.json) and
[compact analysis](inclusive-vacuum-jets.json).

All 1,500 native events pass the topology/boundary audit, including 198 junction
events. Maximum momentum-component residual is 3.31244e-7 GeV. The existing
energy-scaled conservation check passes; the separately reported stricter
component diagnostic has 11 residual excursions. These are visible in the
[audit](validation-inclusive-vacuum-audit.json), not silently waived.

This is an updated vacuum comparison only. It does not establish matched medium
jet/substructure agreement, all production-channel support, or the full goal.
The report JavaScript was syntax-checked; no browser visual-verification claim is
made for this local HTML artifact.
