# Jet mass and Soft Drop analysis

The hepyy/cppyy analysis now records jet mass, groomed mass and zg in addition
to pT, girth and pTD. No standalone SWIG FastJet dependency is used. Production
C++ generation is unchanged by this analysis extension.

## Frozen definition for the forthcoming comparison

Cluster all final visible particles with anti-kt R=0.4, excluding neutrinos;
then select jet pT >= 10 GeV and |eta| < 2. Recoil subtraction and projection
follow the run header, as before. This analysis does not perform hard-parton
matching; it measures inclusive jets.

Recluster all jet constituents with Cambridge/Aachen, E-scheme recombination,
R=1000 to retain one complete tree; reject an unexpected multiple-root result.
Follow the harder-pT parent until min(pT1,pT2)/(pT1+pT2) > 0.1. These are Soft
Drop beta=0, zcut=0.1, R0=0.4 settings. Equality fails the cut. Untagged jets
have null zg and groomed mass and are counted separately. Thus the groomed-mass
histogram includes tagged jets only; it does not place untagged jets at zero.
Mass is FastJet's signed invariant mass, retaining negative values where present.

Mass bin edges in GeV: -50, 0, 1, 2, 4, 6, 8, 12, 20, 40, 80.
zg bin edges: 0.1 to 0.5 in steps of 0.05, with the final upper edge extended
by one representable double so exactly symmetric splits are included.
All observables expose in-range, underflow, overflow and undefined counts and
weighted counts. Histograms retain event-level errors and normalization per
attempt, including failed attempts in the denominator. These definitions are
recorded before a Fortran ratio comparison, not selected to improve agreement.

## Verification

`tests/jet_substructure.py` checks analytic two-body mass/zg, rejection of a
soft outer branch, a singleton, and the strict equality boundary. With
`--contrib`, it independently compares 200 deterministic jets against installed
FastJet-contrib 1.054 SoftDrop, through hepyy/cppyy FastJet 3.5.1. Jets have
1, 2, 3, 8 or 30 constituents, massless/pion/proton masses, varying rapidity,
and both ordinary and wrapped azimuths. Tag decisions agree in all cases;
tagged masses and zg agree at abs=2e-13, rel=2e-12. All checks pass.

The three projection-mode integration samples each retain five selected jets;
all earlier pT, girth and pTD bins are unchanged, and every observable's counts
and weighted bins close. The historical larger native samples were also
reanalyzed after verifying their input hashes against the prior report:

| Sample | Selected jets | Tagged | Untagged |
|---|---:|---:|---:|
| Vacuum | 1081 | 1075 | 6 |
| Medium | 1295 | 1293 | 2 |

All prior pT, girth and pTD distributions remain identical. Observable counts
and weighted bins close. The original run settings and historical generator
hash are retained; these events were not regenerated with the current engine.
The [selectable report](native-substructure-report.html) and
[machine-readable evidence](native-substructure-ensemble.json) include the new
observables, selection, input hashes, analysis source hashes and accounting.
Generated report JavaScript passes a syntax check; it has not been visually reviewed.

This validates analysis operations, not JEWEL generator equivalence. The
matched Fortran samples, ratio uncertainties and 0.9–1.1 acceptance gate remain
outstanding. Historical medium cascade failures and incomplete generator
features are not resolved by adding substructure analysis.

Run with the hepyy environment and hepyy-utils available:

```sh
python tests/jet_substructure.py --contrib
python tools/analyze_native_events.py INPUT.jsonl --output analysis.json
python tools/render_native_report.py analysis.json report.html
```
