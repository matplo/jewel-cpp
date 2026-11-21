# Million-jet vacuum comparison

The native and Fortran runs use 5.02 TeV pp dijets, a 20–100 GeV hard-process
window, CT14nlo/0, ISR, MPI, MPI final-state evolution, vacuum transport,
and final hadrons. The native runner uses its two-stage MPI initializer,
the common-scale Pythia8 coupling translation, the mapped fragmentation profile,
and the JEWEL table-energy setting. The Fortran run uses the frozen JEWEL 2.6.0
executable and cards from the preceding aligned vacuum study with NEVENT
raised to 760,000. Both use seed/job number 2600421, with separate engine
random streams.

The matched settings define the comparison. The engines still differ in the
hard process, ISR, beam-remnant construction and fragmentation. The native
MPI runner uses its existing remnant recoil approximation. Common hepyy and
FastJet analysis clusters full phase space with anti-kt R=0.4 and selects jets
at 10 GeV and above with |eta| < 2. Neutrinos are excluded and all yields are
per original attempted event. The Fortran discard ledger is retained, with
2,128 discards and zero HepMC conversion losses.

The native run command below uses compact output. `--summary-only` in the
analyzer keeps counts and event-level first and second moments in memory.
Those moments preserve the existing histogram standard errors without
retaining each event in the analysis output.

```sh
build-pythia8/jewel-run-events native.jsonl 750000 2600421 vacuum \
  --native-mpi --compact-hadrons --lhapdf CT14nlo/0 \
  --hard-alpha-s .11754434294459773 --hard-alpha-s-order 1 \
  --isr-alpha-s .11754434294459773 --isr-alpha-s-order 1 \
  --alpha-s-nfmax 5 --isr-alpha-s-cmw off \
  --jewel-fragmentation --table-energy jewel --longitudinal-boost on

python3 tools/analyze_native_events.py native.jsonl reference.jsonl \
  --summary-only --output jets.json
python3 tools/compare_jet_samples.py jets.json comparison.json --pair 0:1
python3 tools/render_jet_comparison.py comparison.json comparison.html
```

## Full-sample results

| Sample | Attempts | Written events | Selected jets | Jets per attempt |
|---|---:|---:|---:|---:|
| C++ with native MPI | 750000 | 749989 | 1038388 | 1.38452 |
| JEWEL 2.6.0 Fortran | 760000 | 757872 | 1013731 | 1.33386 |

Eleven C++ attempts failed the hadronization momentum closure check and remain
in the denominator. The reference clustering consumed the complete imported
stream. Its SHA-256 matched the finalized reference JSONL file.

The overall selected yield ratio is 1.038. All eight zg bins pass the 10%
check, with ratios from 1.028 to 1.051. Each of the other five observables
fails at least one bin. The pT ratio is 1.126 ± 0.003 at 10–15 GeV. All bins
from 15–100 GeV are within 10%. The jet-mass ratios are 1.418 ± 0.011 at
1–2 GeV, 0.922 ± 0.003 at 6–8 GeV, and 0.895 ± 0.005 at 8–12 GeV.
The pTD ratio is 1.142 ± 0.008 at .6–.7. Errors are one standard error
from independent event samples.

## What the common-input control shows

In the [paired hadronization test](validation-paired-hadronization.md),
Pythia8 reduces the 8–12 GeV mass yield by 0.0117 ± 0.0031 per event relative
to Pythia6 on the same parton records. The full native–Fortran deficit is
0.00991 per attempt in this bin. The shift is therefore comparable in
size and direction, supporting a substantial contribution from hadronization
and decays for this bin.

For mass 6–8 GeV the paired shift is -0.00456 ± 0.00506 per event, versus a
full deficit of 0.01819 per attempt. For pTD .6–.7 the paired shift is
+0.00256 ± 0.00301, versus a full excess of 0.00773 per attempt. The
common-input comparison does not resolve a shift in either of those bins.
It conditions on the native parton ensemble, so these contrasts cannot
uniquely decompose the full generator difference.

See the [interactive comparison](jewel-million-comparison.html),
[bin ratios](jewel-million-comparison.json), and
[aggregate jet counts and analysis provenance](jewel-million-jets.json).
