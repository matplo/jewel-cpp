# Larger ISR/MPI stability sample and parton diagnostics

Following the imported-fraction and algebraic-zero collision corrections,
1,000 attempts were generated in each of vacuum and medium with seed 2600417.
Settings remain PP at 5020 GeV, pThat 20–100 GeV, ISR+MPI enabled, FSR/hadronization
disabled, angular ordering 0, first-angle ordering off, recoil retention on,
rescattering/longitudinal boost off and 100 GeV table energy. Full run settings,
executable/input hashes and analysis versions are in the evidence JSON.

| Outcome | Vacuum | Medium |
|---|---:|---:|
| Attempts | 1,000 | 1,000 |
| Completed cascades | 953 | 952 |
| Junction colour-partner failures | 32 | 32 |
| Unsupported non-QCD-pair failures | 14 | 14 |
| Frame-mapping failures | 1 | 1 |
| Cascade discards | 0 | 1 |
| Nonfinite cascade failures | 0 | 0 |
| Selected parton jets | 1,646 | 5,034 |

The medium discard is attempt 948 at RNG draw 101,707. It remains in the failed
weight, not the accepted sample. Accepted/failed weights are 953/47 and 952/48;
each generator weight sum is 1,000. The input failures are the known topology
and frame cases; the numerical fixes do not bypass them.

## Common hepyy analysis

The existing hepyy/cppyy FastJet analysis now accepts the native parton schema
in addition to hadron samples. Parton input comprises final statuses 3, 4 and 5;
history, anchors and scratch records are excluded. Hadron input retains its
existing final-particle selection. Neutrinos are excluded in both modes.

Clustering is full phase space, anti-kt R=0.4, followed by pT >= 10 GeV and
|eta| < 2. Girth, pTD, mass and Soft Drop observables use the same implementation
as the existing hadron analysis. Medium recoils are included without thermal
subtraction, so the larger medium jet count is not evidence of a physical
medium enhancement. These are explicit parton-level diagnostics.

All attempted events remain in weighted per-attempt distributions. The analysis
checks complete attempt ordering, summary counts and generator weight accounting.
The existing Fortran/hadron ratio builder rejects parton samples; a regression
check protects that boundary. All seven comparison contracts pass.

[Interactive sample browser](stability-parton-browser.html) provides setting and
observable selectors plus CSV export. Particle level is displayed explicitly.
The generated JavaScript passes syntax validation. [Evidence JSON](validation-stability-parton.json)
includes distributions, under/overflow accounting, failures, run metadata and
provenance; detailed per-event analysis remains in the temporary source artifacts.

This sample exceeds 1,000 selected **parton** jets per setting but does not
satisfy the goal's matched Fortran/native jet comparison. Hadronization, junction
and non-QCD support, the frame failure, the residual cascade discard, matched
configuration/normalization and the final 10% jet/substructure gate remain open.

## Reproduce

Run from the standalone repository in an environment with hepyy/cppyy and
hepyy-utils installed. Use new sample paths.

```sh
build-pythia8/jewel-run-partons /tmp/medium1000.jsonl 1000 2600417 medium --experimental-isr-mpi
python tools/analyze_native_events.py /tmp/medium1000.jsonl --output /tmp/medium-jets.json --generator build-pythia8/jewel-run-partons
python tools/render_native_report.py /tmp/medium-jets.json /tmp/medium-browser.html
```
