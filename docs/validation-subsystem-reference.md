# Full initialized-record replay against JEWEL 2.6.0

The reference harness now accepts `SEQINIT`: a complete prepared record,
explicit ordered subsystem pairs, primary hard scale, run settings and a
reproducible random stream. It derives TYPE1/TYPE2 from the primary pair and
calls the unchanged, hash-verified JEWEL FINDVIRTUALITIES routine first for
the primary and then for each MPI pair. No extracted physics routine changed.

The native diagnostic exports these inputs before initialization and all 26
fields of every initialized output record. Comparison includes integer/status
fields, scheduling links, momenta, masses, auxiliary vertices, production/decay
times, splitting metadata, colours and evolution times. RNG draw counts must
match exactly; discrete fields must match exactly; other fields use the
existing relative tolerance 2e-12 and absolute tolerance 2e-13.

| Seed | Replayed events | Pairs | Compared values | Maximum absolute difference |
|---|---:|---:|---:|---:|
| 2600417 | 953 | 6,356 | 1,483,924 | 1.211e-11 |
| 2600418 | 945 | 6,472 | 1,511,822 | 4.115e-11 |

All **2,995,746 values pass**, with exact draw counts in all 1,898 events.
The previous primary/MPI differential suites also pass after the harness
extension. No production physics implementation or tolerance changed.

The maximum per-pair four-momentum residual is independently calculated from
both outputs. Native and Fortran maxima are identical: 1.0532684200370568e-6
GeV for seed 2600417 and 2.7015437353838934e-7 GeV for seed 2600418. Thus the
observed maxima are reproduced by the source arithmetic on these same inputs;
this does not assert exact conservation for every mapped record.

[Machine-readable evidence](validation-subsystem-reference.json) records the
source manifest, fixture/executable hashes, tolerances and both replay reports.

## Limits of the comparison

Both implementations receive the same candidate Pythia8-derived record and
explicit pair order. This validates the combined initialization operation, not
Pythia6/Pythia8 event-history equivalence or source string/MPI discovery order.
The 70 junction-colour failures, 29 non-QCD-pair failures and three frame failures
from the full 2,000-attempt ledger remain unresolved. Only successfully
initialized events enter this replay; no failure was promoted to an accepted
event. Full cascade and final-jet validation remain pending.

## Reproduce

The diagnostic's `--initialize` mode additionally writes
`FIXTURES.initialization`; it refuses an existing sidecar. Use fresh paths.

```sh
cmake --build build -j 4
cmake --build build-pythia8 -j 4
build-pythia8/jewel-native-preparation /tmp/seq.jsonl /tmp/seq.fixtures 1000 2600417 --initialize
python3 tests/subsystem_reference_audit.py build/jewel-medium-reference /tmp/seq.jsonl /tmp/seq.fixtures.initialization /tmp/seq-audit.json
ctest --test-dir build -R '^(hard|mpi)-initialization-differential$' --output-on-failure
```
