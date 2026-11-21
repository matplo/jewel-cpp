# Native primary and MPI initialization sequence

`initialize_imported_subsystems` connects prepared native subsystem pairs to
the validated FINDVIRTUALITIES translation. It initializes the primary pair
first, then MPI pairs in Pythia8 subsystem order. The primary pThat comes from
the captured subsystem, not an independent caller value.

The adapter derives TYPE1/TYPE2 once from the primary final flavours (gluon to
GC, quark to QQ) and carries those channels through every MPI call. This matches
the global-channel context established by stock `findspeclines`; MPI flavours
do not replace it. MPIFSR and first-angle settings remain caller inputs.

All pairs are validated before drawing: subsystem indices must be contiguous,
hard IDs distinct and present in the source map, roles consistent, the primary
legs active, and all legs quarks or gluons. Unsupported non-QCD pairs fail
explicitly. This is a current limitation, not a claim to support all Pythia8
MPI channels. The whole state commits only when every pair succeeds. The RNG
is not rewound on a failure after draws have begun.

## Checks

Twenty generated ISR+MPI events (Pythia8 8.317, seed 2600417, PP at 5020 GeV,
pThat 20–100 GeV, FSR/hadronization off) pass the new sequencing contract:

- 20 primary and 111 MPI pairs initialized;
- 74 MPI legs have a flavour-derived channel differing from the inherited
  primary channel, exercising the global-channel rule;
- explicit ordered calls to the pair initializer reproduce the adapter's
  momenta, masses, fractions, angles, decay times and random-draw counts exactly;
- exhaustion at the first MPI draw after a completed primary initialization
  leaves the entire input state unchanged;
- unprepared records and unsupported non-QCD MPI pairs fail before draws;
- scheduling parents and ancestry survive the sequence.

This sequencing contract is not an independent Fortran full-event comparison.
The pair operations it invokes retain independent Fortran differential coverage:
288 primary cases / 45,792 values and 1,152 MPI cases / 183,168 values. Both
suites pass after this integration. Native input, boundary, hard-initialization
and subsystem-initialization contracts also pass.

## Remaining scope

The full event runner still rejects ISR/MPI. Pythia8 subsystem order and pair
identity intentionally replace the source's parent-zero / pT-matching discovery
heuristic; equivalence to source string order is not established. Generated
full-record Fortran replay, non-QCD pairs, initialization/cascade integration,
known frame failures and the final >=1,000-jet ratio requirement remain open.

```sh
cmake --build build-pythia8 -j 4
ctest --test-dir build-pythia8 -R '^(subsystem-initialization|pythia8-input|pythia8-boundary|hard-initialization)-contracts$' --output-on-failure
ctest --test-dir build -R '^(hard|mpi)-initialization-differential$' --output-on-failure
```

## Larger full-input-chain diagnostic

The [2,000-attempt diagnostic](validation-initialization-sample.md) now connects
ISR timing, survivor preparation and this initialization sequence. It identifies
junction colour-angle handling and non-QCD MPI pairs as concrete remaining gaps.

The sequence now also has [full-record Fortran replay](validation-subsystem-reference.md)
for the 1,898 supported events from that diagnostic. It does not resolve the
junction, non-QCD, source-order or full-cascade gaps.

An explicit [experimental parton cascade mode](validation-experimental-cascade.md)
now connects this sequence to evolution. It does not resolve the remaining
physics gaps or enable the hadronized runner.
