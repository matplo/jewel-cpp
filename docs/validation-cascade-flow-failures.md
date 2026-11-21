# Investigating the two flow-enabled cascade failures

Recorded 2026-11-21. This investigation replays attempts 4 and 18 from the
20-attempt flow-enabled sample using identical initialized records and RNG
cursors. It does not change physics, relax tolerances or remove failed events
from accounting.

## Optional diagnostics

`Cascade::evolve` now accepts an optional read-only observer of branch
boundaries. The native probe enables stderr tracing with
`JEWEL_CASCADE_TRACE=1`; the guarded Fortran scheduler supports the same flag.
The extracted Fortran physics routines remain unchanged. Traces record the
branch call, scheduling slot, record count, RNG cursor and discard flag. Before
each branch they also record the selected particle's momentum, mass, fraction,
evolution time, production time and decay time. No after-callback occurs when
a native branch throws; the Fortran guard checks finiteness after MAKEBRANCH.

Both engines' stdout is byte-identical with and without tracing for both cases.
The existing cascade-native-input differential test passes after this change.
These checks verify that diagnostics did not change these event trajectories.

Reproduce from the repository root, with the reference build available:

```sh
JEWEL_CASCADE_TRACE=1 python tools/replay_native_cascade.py \
  build/jewel-medium-probe build/jewel-medium-reference \
  docs/cascade-flow-failure-traces /tmp/new-flow-failure-replays \
  --attempts 4 18 --timeout 45
python tools/compare_cascade_traces.py \
  /tmp/new-flow-failure-replays/18.jewel-medium-probe.err \
  /tmp/new-flow-failure-replays/18.jewel-medium-reference.err \
  /tmp/flow-trace-comparison.json
```

The trace comparator reports the first positional difference and separately
aligns entries by scheduling slot, record count, RNG cursor and discard flag.
Alignment cannot prove equality of unobserved event records or intermediate
operations within a branch. It is a diagnostic, not a physics parity gate.

## Attempt 4: shared kinematic rejection

All 29 corresponding branch-entry states agree within rel=2e-12, abs=2e-13.
Both engines discard the event at branch 29, scheduling slot 49, with 61
records and RNG cursor 14,967. The Fortran diagnostic explicitly says that
MAKESPLITTING found no kinematically allowed splitting and rejects the event.
The selected entry is a massive parton (mass about 1.879 GeV, fraction 0.907).
This is evidence of a shared rejection, not a successful physical event.

## Attempt 18: shared failing branch, different stopping points

The first positional trace difference occurs at call 43: Fortran revisits slot
90, while C++ advances to slot 92. Fortran later revisits slot 96 as well.
Each extra visit consumes zero random numbers and changes no record count.
The revisited entries have evolution time 62.09600962513619 fm, at the lifetime
boundary to floating-point precision. Floating-point finalization is a plausible
explanation for the extra visits; this is not a proved inner-operation cause.

After aligning branch entries, all 78 common entries agree within tolerance.
Both engines reach slot 152 with 166 records and RNG cursor 170,223. The
selected massless parton's energy is about 1.24807 GeV. C++ throws
`nonfinite kinematics input` inside that branch at cursor 170,901. Fortran
continues within MAKEBRANCH until the post-branch guard finds nonfinite values,
with 178 records and cursor 170,910.

Thus the different final counts do not by themselves demonstrate an earlier
stochastic-history divergence. Both encounter trouble in the corresponding
branch from matching observed entry states. The exact inner numerical cause
and full-record equivalence before that branch remain unproven; neither result
is counted as passing parity. Do not clamp or retune the physics to hide it.

## Evidence

- [Run-level results and unchanged-output hashes](validation-cascade-flow-failures.json)
- [Attempt 4 comparison](cascade-flow-failure-traces/4.json)
- [Attempt 18 comparison](cascade-flow-failure-traces/18.json)
- Input fixtures and complete stderr traces are preserved beside those reports.

The full-generator feature and >=1000-selected-jet comparison requirements
remain open. This work explains two failures in the small flow-enabled sample;
it does not establish an acceptable failure rate or full JEWEL equivalence.
