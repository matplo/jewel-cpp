# ISR recoil reconstruction: source-matrix diagnosis

The three remaining frame failures in the translated-coupling sample are
attempts 385, 1089 and 1355. This diagnostic traces the actual Pythia8 8.317
SimpleSpaceShower matrix immediately after `Mtot.rotbst(MfromRest)` and the
native reconstructed matrix before its recoil-copy check.

Only temporary source copies and a temporary library/executable were built.
The installed Pythia library and production reconstruction code were unchanged.
Both the source-only trace and the combined source/native trace reproduce the
baseline 1500-event hard-input JSONL **byte for byte**, including failures.
The same seed, PDF, coupling options and ISR/MPI settings were used.

| Attempt | Largest source matrix entry | Largest matrix reconstruction difference | Recovered azimuth minus source (radians) |
|---|---:|---:|---:|
| 385 | 31229.38 | 8.83e-9 | 3.61e-11 |
| 1089 | 2643.90 | 4.51e-9 | 6.02e-11 |
| 1355 | 5599.92 | 1.03e-7 | -1.08e-9 |

The matrices act on highly boosted momenta, so cancellation amplifies small
input differences. In attempt 385, incoming x values are approximately
1.024e-6 and 0.5415. Squaring the stored evolution scale also differs from
Pythia's original pT-squared by one representable step. Thus particle output
is not a lossless record of the source's intermediate variables.

Applying the captured source matrix through the **compiled native
apply_isr_frame function** passes the existing component tolerance for every
recoil copy in all three offending branches. Reconstructed matrices fail that
same check. Maximum copy errors per branch for reconstructed matrices are
2.78e-8, 5.59e-9 and 7.17e-8 GeV respectively; the gate is component-dependent,
so the largest absolute error need not be the component that fails.
The detailed JSON includes all source/native matrices, per-copy errors,
component-gate results and file hashes.

This identifies a precision loss in recovering source kinematics. It does not
yet prove a replacement reconstruction or justify relaxing the production
check. The three attempts remain rejected, and full-generator agreement is
still unproven. The next implementation should preserve source information or
use a demonstrably stable reconstruction, retaining independent recoil-copy,
Lorentz-metric and frame-composition checks.

## Diagnostic artifacts and reproduction

- [Source trace patch](../reference/isr-source-trace.patch): add logging only to
  a copy of Pythia8 SimpleSpaceShower.cc.
- [Native trace patch](../reference/isr-native-trace.patch): add logging only to
  a copy of adapters/pythia8_isr_frames.cpp.
- [Compiled matrix evaluator](../reference/isr_matrix_apply.cpp): link with the
  current native adapter/core libraries and installed Pythia8 library.
- [Trace analyzer](../tools/analyze_isr_source_trace.py): accepts the diagnostic
  directory and writes report.json after requiring byte-identical output.
- [Saved results](validation-isr-source-trace.json).

The temporary directory is `/private/tmp/jewel-isr-source-audit`. Source object
compilation uses `-O2 -std=c++11 -fPIC`, matching the Pythia build configuration;
the native diagnostic uses `-O3 -std=c++17`. The temporary Pythia library reuses
all original source objects except the instrumented SimpleSpaceShower.o.
The diagnostic runner links the existing hard-input application object and
native libraries, with the instrumented native-frames object preceding them.
The installed Pythia library directory remains in the runtime search path for
the LHAPDF plugin. No physics expression, random draw or veto was added.

Generate baseline1500.jsonl with the installed library, events1500.jsonl with
the source-only diagnostic, and both1500.jsonl with the combined diagnostic.
Use 1500 attempts, seed 2600421, ISR/MPI on, CT14nlo/0, hard/ISR alpha_s
0.11754434294459773 with order 1, nfmax 5 and CMW off. Redirect combined stderr
to both1500.err. Store the evaluator as `apply`, with its source as apply.cpp.
Then run:

```sh
python3 tools/analyze_isr_source_trace.py /private/tmp/jewel-isr-source-audit
```

The analyzer matches branches by mother/emission IDs and both before/after
beam fractions, avoiding accidental association with rolled-back candidates.
The Fortran JEWEL physics engine is not involved in this Pythia8 adapter check;
its existing component validations and unresolved full-cascade comparisons
remain separate.

The subsequent [bounded reconstruction implementation](validation-isr-recovery.md)
recovers the three cases with unchanged recoil tolerances. This diagnostic
records the preceding implementation; its native trace patch applies to
`adapters/pythia8_isr_frames.cpp` at commit ecd7c7c.
