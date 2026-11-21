# Vacuum comparison with translated hard/ISR couplings

The native run uses the explicit source-derived hard/ISR coupling policy, with
1500 attempts and seed 2600421. Other settings match the preceding Lund-profile
comparison: CT14nlo/0, pp at 5020 GeV, hard pT 20–100 GeV, experimental ISR/MPI,
JEWEL fragmentation profile, EOVEST table energy and longitudinal boost on.
The common analysis uses hepyy/cppyy FastJet, anti-kt R=0.4, jet pT > 10 GeV,
|eta| < 2, neutrino exclusion and the unchanged Soft Drop settings and bins.

1496 attempts succeed, yielding **2090 selected jets**. The unchanged Fortran
sample contains **1357 selected jets**, from 997 written events in 1000 attempts.
All attempts, including failures/discards, remain in the normalization.
The global flavour cap affects MPI as well; this is not an isolated ISR-only
experiment, and MPI equivalence remains unproven.

| Jet pT (GeV) | Native / Fortran |
|---|---:|
| 10–15 | 1.183 |
| 15–20 | 0.925 |
| 20–30 | 0.998 |
| 30–40 | 0.966 |
| 40–60 | 1.116 |
| 60–80 | 0.722 |

All six observable gates remain not passed. The report preserves statistical
errors and undefined bins; neither changes the central-value 10% criterion.
No tuning to ratios or analysis-cut changes were applied.

Four failures remain explicit: ISR frame comparisons at attempts 385, 1089 and
1355, and an unsupported subsystem particle configuration at attempt 511.
Their tolerance/rejection checks have not been relaxed. Accepted-event topology
and energy-scaled momentum checks pass; stricter component diagnostics remain
in the saved audit rather than being omitted.

- [Interactive comparison, including historical samples](translated-vacuum-comparison.html)
- [Numerical comparison and configuration provenance](translated-vacuum-comparison.json)
- [Compact common-analysis output](translated-vacuum-jets.json)
- [Topology and failure audit](validation-translated-vacuum-audit.json)

This is progress on configuration alignment, not validation of full-generator
physics parity. Next work must resolve the unsupported subsystem and frame
failures and audit the remaining MPI and shower-scale differences.
