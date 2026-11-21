# Recoil rescattering and momentum cuts

Recorded 2026-11-21. Native runners now accept:

- `--recoil-rescattering on|off`: source SCATRECOIL setting, default off.
- `--recoil-cuts SOFT HARD`: finite, nonnegative dimensionless multipliers
  of 3T, default 0/0 to reproduce earlier native output.

The source default hard multiplier is 5; it is selected explicitly here.
The recoil momentum magnitude is evaluated in the local fluid rest frame.
A recoil below SOFT*3T is excluded from the colour chain/string input. With
recoils kept and rescattering enabled, one above HARD*3T is marked for further
evolution. Source comparisons use strict inequalities. The soft test takes
precedence, so SOFT need not be smaller than HARD. Disabling recoil retention
prevents recoils from entering this evolution path regardless of the switch.

All three values propagate into collision finalization and reference replay.
The header records `recoil_rescattering`, `recoil_soft_cut_3T` and
`recoil_hard_cut_3T`. These settings do not perform thermal subtraction.

```sh
build-pythia8/jewel-run-events rescattering.jsonl 20 2601001 medium replay-inputs \
  --centrality 0 10 --table-energy jewel --longitudinal-boost on \
  --recoil-rescattering on --recoil-cuts 0 5
ctest --test-dir build-pythia8 -R '^runner-recoil-controls$' --output-on-failure
```

## Validation and current limitation

[Machine-readable evidence](validation-recoil-controls.json) preserves audits,
executable hashes, strict reference results and the active-case diagnostic.

The runner regression passes. It verifies omitted/explicit defaults are
identical, all settings reach the replay, invalid/repeated arguments are
rejected, and a very high soft cut excludes recoil sources from hadronization.
It deliberately retains the observed active-mode failure and verifies its RNG
cursor against C++ replay; this check is not a successful physics-parity test.

At hard cut 5, the 20-attempt sample accepts and hadronizes 14 events, with six
cascade failures. The accepted-event accounting, record and hadronization
conservation audit passes. The recoil-off-rescattering baseline accepted 18.
The four newly failed attempts are 7, 9, 12 and 19; all four also become
nonfinite in guarded Fortran. Attempt 9 stops at differing draw counts; the
other three have matching stop cursors. These failures are not parity passes.
Re-exporting the complete sample with replay fixtures gives byte-identical JSONL.

The first two events do not change their cascade activity at hard cut 5.
Their Fortran comparisons repeat the earlier result: attempt 1 has 11 strict
scalar mismatches (max abs 3.32001e-12), attempt 2 passes. They do not establish
that actively rescattered events work correctly.

With hard cut zero, attempt 1 exercises rescattering and fails at RNG cursor
2742. Temporary diagnostic instrumentation finds formation factor 1, colour
maximum 111, and splitting fraction -1 at the native collision-finalization
precondition. Its stdout is unchanged by the diagnostic. Fortran reaches the
same draw count with nonfinite output instead of rejecting at that precondition.
Source DOKINEMATICS computes the splitting angle using sqrt(z*(1-z)) when z is
nonzero; a negative sentinel is outside that expression's finite domain.
This pinpoints the native guard condition and the reference failure, without
claiming that every intermediate field in both branches has been compared.

A very high soft cut (1,000,000) completes one event, excluding recoil sources
from the string input. It verifies the soft-selection path; it is not a useful
physics setting or evidence that active rescattering is validated.

The four cut-5 failing fixtures and the zero-cut fixture are preserved under
`recoil-rescattering-fixtures/`. The latter is named `zero-threshold-1.input`;
feed it directly to a probe rather than the numeric-filename batch replay tool.

No clamp, sentinel reinterpretation or failed-event filtering has been added.
A successful actively rescattering hadronized sample has not been established
by these checks. This mode therefore remains experimental; full feature
validation and the >=1000-jet, 10% comparison gate remain open.
