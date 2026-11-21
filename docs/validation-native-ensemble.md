# Native hadronized samples and jet analysis

Recorded 2026-11-21. This checkpoint exercises the reduced native PPJJ chain at
more than 1,000 selected jets in each setting. It does **not** meet the requested
Fortran/C++ physics-comparison gate: no Fortran sample is included in this report.

[Browsable distributions and failures](native-hadron-report.html) ·
[Analysis data and per-event jets](native-hadron-ensemble.json) ·
[Record/conservation audit](native-hadron-ensemble-audit.json).

| Setting | Attempts | Accepted events | Cascade failures | Selected jets |
|---|---:|---:|---:|---:|
| Vacuum | 1,000 | 1,000 | 0 | 1,081 |
| Expanding medium | 1,500 | 1,364 | 136 | 1,295 |

The initial 1,000-attempt medium run produced only 891 selected jets. The extended
run reproduces every one of its first 1,000 attempt records byte-for-byte. Only
the extended medium run is used in the report; the overlapping samples are not
added together. Both settings use seed 2601001 and the fixed reduced settings
recorded in [the native runner documentation](validation-strings.md).

All accepted cascades completed HADROTYPE=0 string construction and separate
Pythia8 hadronization. Medium failures comprise 90 nonfinite-kinematics inputs,
43 nonfinite shower records and three explicitly discarded cascades. None is
replaced by another accepted event. The 9.07% failed-attempt fraction means
acceptance bias remains a material unresolved issue for physics interpretation.

## Analysis definition

Jet finding uses `hepyy_utils.fastjet.require_fastjet()`, backed by hepyy/cppyy
and native FastJet 3.5.1. There is no standalone SWIG FastJet dependency.
All final visible particles enter anti-kt R=0.4 clustering across full phase
space; neutrinos with absolute PDG ID 12, 14 or 16 are excluded. Only after
clustering are jets selected with pT >= 10 GeV and |eta| < 2.

Girth is the constituent-pT-weighted mean distance to the jet axis in eta/phi.
pTD is sqrt(sum constituent pT²)/sum constituent pT. Every selected jet falls
inside each observable's recorded histogram range; no selected jet was lost to
an unreported overflow in these samples.

Medium recoils are included and thermal subtraction is **not** applied. The
plots therefore are not a background-subtracted quenching result. Histograms
show weighted jets per attempted event per bin, without bin-width division or
cross-section normalization. Failed attempts contribute zero. Error bars use
variation between whole events, not independent-jet Poisson errors.

## Integrity and provenance

The audit streams the files rather than retaining all particles in memory. It
checks finite numbers, unique stable IDs, ancestry, final cascade statuses,
source mapping, colourless final particles, hadronization momentum closure,
complete attempt sequences and weight/count closure. Native hadronization also
checks electric charge conservation on each accepted event.

Vacuum contains 108,575 final particles; medium contains 356,974. The largest
hadronization momentum-component residual is 1.94856e-7 GeV, within the unchanged
energy-scaled conservation tolerance. Input SHA256 hashes, generator executable
hash, generator settings and analysis-library versions are recorded in the JSON.

The self-contained HTML allows selection of setting and observable, displays
failure reasons and provenance, and exports the selected histogram as CSV.
Its JavaScript syntax and histogram coverage were checked; browser rendering
has not been visually reviewed in this checkpoint.

## Reproduction

```sh
build-pythia8/jewel-run-events vacuum.jsonl 1000 2601001 vacuum
build-pythia8/jewel-run-events medium.jsonl 1500 2601001 medium
python tools/check_parton_run.py vacuum.jsonl medium.jsonl --output audit.json
# Run in an environment containing hepyy[cppyy,hepbundle] and hepyy-utils:
python tools/analyze_native_events.py vacuum.jsonl medium.jsonl \
  --generator build-pythia8/jewel-run-events --output analysis.json
python tools/render_native_report.py analysis.json report.html
```

No change to physics code or generator settings was made for these larger runs.
The next comparison must include a frozen Fortran sample with explicitly matched
hard-process/PDF, shower, medium, recoil and hadronization settings. Fragmentation
differences between Pythia6 and Pythia8 must be separated from core-port errors.
Known cascade trajectory discrepancies, incomplete feature coverage and custom
outputs remain open. Neither the ±10% comparison nor full JEWEL completion is
claimed by the native sample size.
