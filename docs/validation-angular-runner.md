# Angular-ordering selection in native generation

Recorded 2026-11-21. Both native runners accept `--angular-ordering 0..5`,
using the existing source-derived branching and splitting modes. The selected
integer configures the shared BranchingSampler, the BCONFIG replay setting,
and the run header's `angular_ordering` field. Omission preserves mode 0.
Stock JEWEL initializes ANGORD=1; select it explicitly for comparisons.

The independent `AOFIRSTSPLIT` setting remains false and is now explicitly
recorded as `first_angle_ordering: false`. Enabling that option still requires
mapping the hard-input ancestry; ANGORD does not silently enable it.

```sh
build-pythia8/jewel-run-events angular.jsonl 20 2601001 medium replay-inputs \
  --centrality 0 10 --table-energy jewel --longitudinal-boost on \
  --angular-ordering 1
ctest --test-dir build-pythia8 -R '^runner-angular-contracts$' --output-on-failure
ctest --test-dir build -R '^(branching|splitting|rejection)-differential$' \
  --output-on-failure
```

## Passing checks

The three mode-dependent component suites pass against frozen JEWEL 2.6.0.
All six modes complete and hadronize two medium events, with accounting and
conservation audits passing. Modes 3, 4 and 5 alter these initial histories.
The integration regression checks all six selected modes against separately
configured C++ replay: particle momenta, statuses and RNG cursor agree exactly.
Omitted mode and explicit mode 0 are identical, and invalid/repeated arguments
are rejected before creating output.

The mode-1 20-attempt smoke sample accepts 19 events. Compared with mode 0,
several histories change; this is expected when changing the physical setting,
not evidence of improved agreement or a resolved failure rate.

## Strict Fortran event comparisons remain incomplete

For the first event of each mode, the record, scattering-centre and random-draw
counts match, but each strict comparison has scalar differences:

| Mode | Mismatches outside tolerance | Maximum absolute error |
|---|---:|---:|
| 0 | 11 | 3.32001e-12 |
| 1 | 11 | 3.32001e-12 |
| 2 | 11 | 3.32001e-12 |
| 3 | 47 | 7.21201e-12 |
| 4 | 47 | 7.21201e-12 |
| 5 | 9 | 4.31100e-12 |

The tolerance remains rel=2e-12, abs=2e-13. These are not strict parity passes.
In the combined first-event report, fixture IDs 1–6 correspond to modes 0–5;
each fixture represents generated attempt 1. A further mode-1 event (attempt 3)
has matching counts but three scalar mismatches, max abs 1.50093e-11.

### Unresolved finite-reference discrepancy: mode 1, attempt 9

C++ fails at RNG cursor 1791 with nonfinite kinematics. The guarded Fortran
replay completes with 192 records, 64 centres and cursor 20090. Unlike the
shared failures investigated earlier, this is a failed native trajectory with
a finite reference outcome and must be resolved before full validation.

Temporary collision diagnostics find one negative squared momentum in C++:
`p32=-6.4810197805030745e-18`, with `p42=0`, `E4new=1.0876602521270242` and
`m42=1.183004824057022`. No negative p32/p42 diagnostic is emitted by Fortran.
The native backtrace enters the next collision candidate's rotation with
nonfinite momentum. Both diagnostic builds preserve their respective baseline
stdout exactly. This localizes a roundoff-sensitive difference; it does not yet
prove the complete preceding state or identify the correct faithful remedy.
No clamp or relaxed parity tolerance was introduced.

The input is preserved at [angular-mode1-regression/9.input](angular-mode1-regression/9.input).
Reproduce independently of the Pythia8 generator:

```sh
python tools/replay_native_cascade.py build/jewel-medium-probe \
  build/jewel-medium-reference docs/angular-mode1-regression \
  /tmp/new-angular9-replay --attempts 9 --timeout 45
python tools/diagnose_collision_roundoff.py build \
  docs/angular-mode1-regression/9.input /tmp/new-angular9-diagnostic
```

[Machine-readable validation](validation-angular-runner.json) includes all
component reports, native audits, comparison details, hashes and diagnostic
operands. No full-generator jet-spectrum or substructure parity is claimed.
