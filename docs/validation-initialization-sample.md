# Full native input-chain diagnostic

The preparation probe now accepts `--initialize`, connecting native import,
ISR timing attachment, ordered survivor preparation and primary/MPI virtuality
initialization. Its output preserves the preparation reference fixtures and
adds initialization outcomes, independent RNG seeds/draw counts, initialized
pair counts, active-record counts and maximum per-pair four-momentum residual.
The final-state cascade is not run.

Two 1,000-attempt samples use seeds 2600417/2600418 with the established PP
5020 GeV, pThat 20–100 GeV, ISR+MPI settings. Initialization uses primary pThat,
MPIFSR enabled, first-angle ordering disabled, NF=3, Q0=1 GeV, Lambda=0.2 GeV,
formation factor 1 and lifetime 20 fm. Its SplitMix seed is
`generator_seed * 100000 + attempt`; ISR timing retains the diagnostic 0.5 tape.

| Seed | Initialized | Pairs initialized | Junction-angle failures | Unsupported non-QCD pairs | Frame failures |
|---|---:|---:|---:|---:|---:|
| 2600417 | 953 | 6,356 | 32 | 14 | 1 |
| 2600418 | 945 | 6,472 | 38 | 15 | 2 |

All attempts remain in the ledger. The 99 initialization failures consume no
initialization draws. They are failures, not accepted events or silently removed
contributions to a jet spectrum.

## Findings that determine the next work

All 70 missing-colour-partner failures are explained by junction endpoints:
the 73 missing primary hard-leg colour tags all occur in captured Pythia8
junction records. The pairwise GETTHETAM mapping does not yet define a junction
opening angle. No artificial partner or angle was supplied to bypass this gap.

The 29 unsupported-pair events contain outgoing PDG 22, 445, 10441 or 20443.
The current initializer intentionally requires quark/gluon pairs. Extending it
requires the source's retained-mass/nonparton branches and treatment of their
times; substituting zero masses would not preserve the source behavior.

Maximum absolute per-pair four-momentum residuals are 1.05327e-6 and 2.70154e-7
GeV for the two samples. These are measured diagnostics, not a passed reference
agreement gate. Generated full-record initialization replay remains needed to
separate source arithmetic from mapping effects.

The unchanged preparation stage still passes all 640,920 Fortran comparisons;
its fixture files are byte-identical to the previous preparation-only samples.
This does not extend that reference claim to initialized records or cascades.

[Machine-readable evidence](validation-initialization-sample.json) records all
failures, source-record classification, hashes and preparation replay results.
The full >=1,000-selected-jet / 10% ratio requirement remains outstanding.

## Reproduce

With fresh paths:

```sh
cmake --build build-pythia8 -j 4
build-pythia8/jewel-native-preparation /tmp/initialized.jsonl /tmp/initialized.fixtures 1000 2600417 --initialize
python3 tests/native_preparation_audit.py build/jewel-kinematics-reference /tmp/initialized.jsonl /tmp/initialized.fixtures /tmp/preparation-audit.json
python3 tests/subsystem_initialization_audit.py /tmp/initialized.jsonl /path/to/matching-captured-snapshots.jsonl /tmp/initialization-audit.json
```

The last audit requires the matching Pythia8 snapshot capture, including
subsystems, final-copy history and junctions; it verifies run-setting identity
and a complete attempt ledger before classifying failures.
