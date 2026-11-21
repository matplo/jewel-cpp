# Vacuum comparison with source-derived diquark fragmentation

The 1,500-attempt native run uses the same seed 2600421, hard/ISR/MPI controls,
PDF, table energy and shower parameters as the preceding inclusive sample.
All events complete. Every pre-hadronization event record and summary is exactly
unchanged; only final hadrons differ. The only run-header changes are the
hadronizer settings: explicit pi0 decay on (the existing effective default),
aExtraDiquark=0.5, aExtraSQuark=0 and useOldAExtra=off.
[Paired identity evidence](validation-lund-paired.json).

Common hepyy/cppyy analysis gives 2,150 native jets and the unchanged 1,357
Fortran jets from 997 written / 1,000 attempted reference events. Settings and
normalization are unchanged: anti-kt R=0.4, pT>10 GeV, |eta|<2, weighted jets per
original attempt, and no shape renormalization. No cut or bin was selected to
improve a ratio.

| Jet pT (GeV) | Corrected profile / Fortran | Previous profile / Fortran |
|---|---:|---:|
| 10–15 | 1.149 | 1.140 |
| 15–20 | 0.962 | 0.924 |
| 20–30 | 1.019 | 1.017 |
| 30–40 | 1.064 | 1.113 |
| 40–60 | 1.362 | 1.290 |
| 60–80 | 1.056 | 1.111 |

The 10% gate still fails. Some bins move closer to unity and others farther
away; this is not evidence that the source-derived correction resolves the
physics discrepancy. Substructure gates also fail, for example zg 0.30–0.35
has ratio 1.241. Sparse and undefined bins remain visible. Statistical error
bars do not relax the central-ratio gate.

The [interactive report](lund-vacuum-comparison.html) now offers the corrected
profile, preceding inclusive profile, and earlier sample with rejected inputs,
with six observables and CSV export. The historical pairs retain their analysis
hashes. [Comparison JSON](lund-vacuum-comparison.json) and
[compact analysis](lund-vacuum-jets.json) include the settings and provenance.
The generated JavaScript was syntax-checked; no local-browser visual check is
claimed.

All 1,500 events pass the native topology and energy-scaled momentum audit.
Maximum momentum-component residual is 1.93162e-7 GeV; the separately retained
stricter component diagnostic has nine excursions.
[Boundary audit](validation-lund-vacuum-audit.json).

## Next configuration-audit priority

The frozen Pythia6 source's PDFALPHAS fallback assigns the card value to ALAM,
then PARP(1), PARP(61), PARP(72) and PARU(112). Its PYALPS routine uses PARU(112)
as Lambda, despite the fallback diagnostic naming alpha_s(M_Z). The existing
reference stdout shows that this fallback was used. Therefore matching a card
label to Pythia8 alphaSvalue=0.118 does not establish identical effective hard or
ISR coupling. The next audit must measure the initialized running couplings and
orders directly; no coupling change was made or claimed here. Full physics
configuration equivalence and the requested jet/substructure agreement remain
unproven.
