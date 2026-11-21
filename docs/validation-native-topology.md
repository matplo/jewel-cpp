# Native junction hadronization boundary

The experimental ISR+MPI runner now passes completed native partons and explicit
Pythia8 junctions to the existing standalone Pythia8 hadronizer. The boundary
preserves final parton momenta, masses, colour tags, source indices and production
vertices. It accepts outgoing junction kinds 1 and 2 and requires every colour
tag to balance, including the junction endpoints. It rejects unfinished cascades,
missing/repeated tags and unsupported junction kinds. Pythia8 showers and colour
reconnection remain disabled. This is a Pythia8 topology mapping, not a claim of
Pythia6 string-order equivalence. Experimental HADROTYPE 1 remains unsupported.

## Integration evidence

Both 20-attempt samples at seed 2600417 accept 19 events, including nine junction
events each. Every accepted boundary has balanced colours. Final hadron four-momentum
agrees with the supplied partons to a maximum component residual below 6.5e-9 GeV.
The complete weight ledger retains the failed event. Synthetic junction and
antijunction fragmentation tests pass, as does a generated six-event regression
checking colour balance, conservation and the known rejection. Both selected
CTest suites pass.

The common hepyy/cppyy analysis selects 25 vacuum jets and 90 medium jets with
anti-kt R=0.4, pT>=10 GeV and |eta|<2 after full-phase-space clustering. Medium
recoils are retained without subtraction. These are integration samples, far
below the requested 1,000-jet threshold; no Fortran/native jet-ratio gate is claimed.

## Rejected event and Fortran replay

Attempt 6 fails colour balance in both samples. Native slot 79 (PDG 2, source
Pythia8 index 172, MPI subsystem 9) carries colour 144 and a true quark-pair flag.
Its daughters 96 and 97 have PDGs -1 and 2 and both colour tags zero. This loses
the junction endpoint. The existing MPI initialization inherits the primary
sampling channels, following the source TYPE1/TYPE2 behavior.

Replaying the identical initialized vacuum state through the frozen JEWEL 2.6.0
routines reproduces the malformed daughters, the 116-record result and all 500
random draws. Every emitted numeric value passes the unchanged relative 2e-12,
absolute 2e-13 tolerances. MAKESPLITTING assigns QQBAR=QQBARD(L) after the quark
branch sets QQBAR false. This demonstrates behavior on this imported state; it
does not demonstrate that stock Pythia6 generates the same input.

No colour repair or sampling-policy change was made to hide this failure.
Resolving flavour-inconsistent MPI initialization, hard legs connected to
junctions, non-QCD MPI and the remaining frame failures is still required.
[Machine-readable evidence](validation-native-topology.json) records the samples,
weights, residuals, input hashes and reference replay outcome.

Reproduce the boundary checks with:

```sh
ctest --test-dir build-pythia8 --output-on-failure \
  -R '^(pythia8-shower-output|experimental-isr-mpi)-contracts$'
```
