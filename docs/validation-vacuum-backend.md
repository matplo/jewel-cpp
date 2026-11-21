# JEWEL vacuum backend

The native runners now distinguish three modes:

- `vacuum`: the frozen JEWEL medium-vac.f contract.
- `medium`: the existing expanding optical medium with scattering.
- `zero-interaction`: the former native vacuum behavior, an expanding background
  with scattering disabled. Centrality remains available in this mode.

This intentionally changes the meaning of the runner's `vacuum` argument.
Use `zero-interaction` to reproduce earlier expanding-background vacuum samples.
The 20-event legacy sample matches exactly in every non-header record under
that name. True vacuum rejects centrality arguments before creating output.
Run headers now record `medium_backend` explicitly.

## Implementation

`MediumModel` owns either the existing ExpandingMedium value or the vacuum
contract. Scattering/transfer objects receive this same backend, and the runner
uses it for table construction, event geometry, initialization and evolution.
Vacuum fields, bounds and lifetime are zero; production vertices are at the
origin and consume no random draws. The source MAXSCATCEN auxiliary value is
preserved: zero three-momentum, energy and mass 0.5 GeV, regardless of boost.
This is a table-initialization value, not a physical medium constituent.

Vacuum collision sampling returns no hit without a random draw. No scattering
grid is built or used. Requests for the nonexistent grid or physical vacuum
scatterers fail explicitly; the source's undefined scatterer outputs are not
invented. The unchanged expanding implementation still supplies all optical
geometry and medium queries.

## Frozen-source validation

`jewel-vacuum-reference` extracts unchanged routines from hash-verified
medium-vac.f.stock and the existing frozen core. The reference extractor
substitutes only matching medium units. The expanding reference rejects VACUUM
fixtures; the vacuum reference rejects CONFIG fixtures. Replay files identify
the backend explicitly. The new reference keeps the existing bounded cascade
scheduler for full-state comparisons.

All vacuum field/table comparisons pass with exact values on this platform:

| Comparison | Values |
|---|---:|
| Fields, bounds, maximum scatterer and RNG-free vertices | 386 |
| Initial Sudakov and partonic PDF tables | 20,000 |
| Full final-splitting tables | 6,004,000 |

Tables cover energies 100 and 846.494030493817 GeV. Relative tolerance remains
2e-12 and absolute tolerance 2e-13; no tolerance or compiler arithmetic policy
changed. [Machine-readable evidence](validation-vacuum-backend.json) includes
source, executable and table hashes.

The 20-attempt native vacuum sample hadronizes all events with zero lifetime,
origin primary vertices and no thermal records. All 20 full-state replays match
headers and random-draw counts. Of 81,674 compared values, 14 remain outside
tolerance in attempts 3, 10 and 20; 17 events pass strictly. The largest absolute
difference among those three events is 4.34364e-12. These are recorded as
failures, not waived. [Full replay report](validation-vacuum-cascade.json).

All 44 existing Pythia8 integration suites passed. The added vacuum contract
also passed, as did the strengthened experimental runner contract. Fifteen
selected expanding-medium/table/transport suites passed; the broad stock
cascade suite timed out in its reference process during the concurrent run.
The isolated rerun also timed out after 240 seconds in the reference process.
Its captured output contains 52 completed fixtures out of 58. Isolating the
next fixture (zero-based case 52, coherent antiquark cascade) reproduces a
60-second timeout both with the current probe and with the unchanged probe
from commit f8ff295 linked against the same expanding-routine objects. Both
produce identical diagnostic stderr. This establishes that the timeout also
occurs without the vacuum-probe changes; it remains unresolved and is not
counted as a pass. [Diagnostic evidence](validation-expanding-cascade-timeout.json).

## Larger true-vacuum comparison

[The interactive comparison](vacuum-backend-comparison.html) now contains 2,088
selected native jets from 1,500 attempts (1,467 accepted) and 1,357 Fortran jets
from 1,000 attempts (997 written, three logged discards). Both use pp at 5020
GeV, CT14nlo member 0, pThat 20–100 GeV, ISR/MPI enabled, the JEWEL EOVEST
prescription and mapped JEWEL fragmentation controls. The Fortran executable
uses the actual vacuum backend; its medium card is unused. The
[reference build manifest](vacuum-reference-build.json) records source hashes
and the previously documented declaration/diagnostic FORMAT fixes.

Both samples use the common hepyy/cppyy analysis and complete per-attempt
normalization. Jet-pT ratios are 1.089 (10–15 GeV), 0.979 (15–20), 0.970
(20–30), 1.039 (30–40) and 1.333 (40–60). The numerical gate still does not
pass, and sparse/undefined bins remain visible. The configuration audit in
[the comparison JSON](vacuum-backend-comparison.json) retains unresolved input,
MPI, colour and fragmentation distinctions. This corrects the vacuum-backend
mismatch; it does not establish complete generator equivalence.

```sh
build-pythia8/jewel-run-events events.jsonl 20 2600417 vacuum replay --experimental-isr-mpi
python tools/replay_native_cascade.py build/jewel-medium-probe build/jewel-vacuum-reference replay comparison
ctest --test-dir build -R '^vacuum-backend-differential$' --output-on-failure
python tools/build_jewel_comparison_reference.py /path/to/frozen/source reference-build --medium vacuum --lhapdf-prefix /path/to/lhapdf
```
