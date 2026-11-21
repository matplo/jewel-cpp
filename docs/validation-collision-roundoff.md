# Shared collision roundoff failure

Recorded 2026-11-21. The inner trigger of flow-sample attempt 18 is now
identified in both engines. This resolves the operation-level uncertainty in
the preceding branch-trace investigation for this particular fixture.

## Operation and evidence

The source DOKINEMATICS mass-reshuffling fallback (stock JEWEL 2.6.0 lines
6055–6092) assigns `p42=0` when `m42` is nonzero, then calculates:

```
E4new = sqrt(p42 + m42)
p32   = E4new**2 - m42
...
p(n,1) = sqrt(p32) * p(n,1) / p3old
```

Temporary diagnostic writes immediately before that final square root find:

| Quantity | Native C++ | Frozen Fortran |
|---|---:|---:|
| p42 | 0 | 0 |
| E4new | 0.42883210077106498 | 0.42883210077105138 |
| m42 | 0.18389697065172483 | 0.18389697065171318 |
| p32 | -9.019477114474516e-19 | -7.992352520205221e-18 |

The negative values equal the exact residuals obtained from the rounded binary
operands `E4new` and `m42`, calculated independently at 80-digit precision.
Computing multiplication and subtraction with separate binary64 rounding gives
zero instead. The native implementation explicitly uses fused multiply-add for
this expression to reproduce the frozen Fortran arithmetic. Taking the square
root of the retained negative residual creates a nonfinite outgoing candidate.

A temporary native backtrace located the later exception in `rotate_boost`,
called from the next collision candidate during `apply_collisions`. This is
consistent with the invalid candidate being carried into the next transfer.
Fortran continues through the branch and its diagnostic guard then detects
nonfinite event records. The two implementations stop at different operations;
neither event is accepted.

These observations establish a shared numerical trigger for this input. They
do not prove every earlier event-record field agrees, explain all failures in
the larger ensemble, or establish full-generator physics parity.

## Reproducibility without changing production physics

The reusable tool requires the existing CMake Unix Makefiles reference build.
It copies and instruments only the native collision source and extracted
Fortran medium reference in a new output directory, reuses their compiler flags
and link commands, and runs each diagnostic and baseline executable on the
same fixture. No source in the production library is edited.

```sh
python tools/diagnose_collision_roundoff.py build \
  docs/cascade-flow-failure-traces/18.input /tmp/new-collision-diagnostic
```

Both diagnostic stdout files are byte-identical to their respective baseline
files. [Machine-readable evidence](validation-collision-roundoff.json) records
all operands, exact residuals, source/executable hashes and compilation commands.
Complete diagnostic stderr is preserved alongside the original branch traces.
The tool rejects a nonunique instrumentation site or changed event output.

No tolerance or clamp has been added to the collision engine. A numerical
stabilization would change this reference trajectory and needs a separately
specified policy and validation against both the frozen baseline and ensemble
observables. It must not be introduced silently to make a test pass.

The broader task remains incomplete: feature coverage, matched generator
settings, the >=1000-jet spectrum/substructure ratio gate and the final browsable
comparison report still require further work.
