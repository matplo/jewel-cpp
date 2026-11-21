# Source ISR matrices separate reconstruction errors from rounding

Recorded 2026-11-21, Pythia8 8.317. The new diagnostic harness records the
actual Mtot matrices and branching inputs inside SimpleSpaceShower. It identifies
two distinct problems in the failed subsystem transport audit: reconstructing
kinematics introduces additional error, and even the original source matrices
can exceed the fixed composition tolerance after further boosts.

## Isolated source observer

`tools/build_pythia_isr_trace.py` verifies SHA-256 hashes of the unmodified
8.317 source and header, generates an independently named shower class in a
**new diagnostic directory**, and compiles a standalone probe against the
installed Pythia library. It retains the upstream source license/comments.
No installed file or production adapter is modified. Generated upstream source
is not included in this repository.

The only branching-code additions copy Mtot and record it after a successful
branch. No random draws or decision expressions are added. The driver selects
the diagnostic shower through Pythia's public ShowerModel interface and filters
trace records against the final emission history. The audit refuses duplicate,
missing or unmatched trace records.

For both seeds 2600417 and 2600418, **every particle's PDG ID, status and four-
momentum in all 2,000 events equals the existing adapter snapshots exactly**.
All 26,962 accepted source emission branches are accounted for. Event weights
and cross sections are not independently exported by this diagnostic driver;
its exact-reproduction claim covers the particle fields listed above.

## Source-matrix composition results

The audit substitutes actual source ISR matrices into the existing staged
transport audit, retaining the validated primordial reconstruction. It uses the
same initial records, energy stage, copy associations and fixed tolerance.

| Seed | Actual source branches | Source-matrix composed failures | Events affected | Maximum error (GeV) |
|---|---:|---:|---:|---:|
| 2600417 | 13,253 | 11 partons | 10 | 4.04482e-7 |
| 2600418 | 13,709 | 8 partons | 5 | 2.53498e-7 |

All individual source-matrix copy comparisons pass. The **composition audit
still fails** its fixed 1e-8 GeV + 1e-10 |p| gate. Neither tolerance nor
production implementation changed. Source replay covers every event, including
the twelve events that the reconstruction API rejects.

The prior reconstructed-matrix audit had 107 composed-parton failures, plus
12 rejected events, and a maximum error of 2.52802e-5 GeV. Actual source matrices
reduce, but do not eliminate, discrepancies. This is evidence for separating
reconstruction error from finite-precision transport behavior, not permission
to dismiss either. The largest source-based residual includes primordial
reconstruction and repeated floating-point application; it is not a bound on
all possible kinematics or spacetime transport.

For the worst prior case (seed 2600417, attempt 785, subsystem 3), the first
ISR transform has infinity norm about 2,973 and introduces a 1.24e-8 GeV copy
residual. The following primordial transform has norm about 5,249; composition
then differs by 2.53e-5 GeV. Amplification is directly observed in this chain.

The recorded source inputs also differ slightly from values reconstructed from
stored vectors. Across both samples, maximum absolute discrepancies are
1.14e-9 GeV² in Q², 1.90e-11 in z, and 1.02e-5 GeV² in m²II. Their maximum
relative discrepancies are below 9.4e-10, 5.4e-11 and 3.1e-11, respectively.
These small input errors can matter in strongly boosted transforms.

A separate direct CM-frame reconstruction from recorded spacelike vectors was
experimentally rejected: it produced more individual-copy failures and did not
resolve composition. It was not added to production code.

[Detailed source replay results, failures and hashes](validation-isr-source-trace.json).

## Reproduction

Build in a directory that does not already exist:

```sh
python3 tools/build_pythia_isr_trace.py PYTHIA_SOURCE PYTHIA_PREFIX NEW_BUILD_DIR
```

Supply exactly the adapter snapshot header's `settings`, one string per line,
then run the generated probe:

```sh
NEW_BUILD_DIR/probe settings.txt trace.jsonl 1000 PYTHIA_PREFIX/share/Pythia8/xmldoc
python3 tools/audit_pythia_isr_trace.py trace.jsonl matching-ancestry-snapshots.jsonl audit.json
```

The audit exits nonzero when composition fails, after recording evidence. That
is the expected outcome of the current samples, not a passing test.

Next work should preserve source branching inputs accurately and evaluate a
source-based numerical error budget. Physical timing and MPI evolution remain
open; this diagnostic does not validate JEWEL 2.6.0 jets or alter Fortran kernels.
