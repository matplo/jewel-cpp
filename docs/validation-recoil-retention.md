# Recoil retention in native generation

Recorded 2026-11-21. Both native runners accept `--keep-recoils on|off`.
The default is on, preserving previous native samples. This maps to stock
JEWEL's KEEPRECOILS parameter / ALLHAD flag; it does not enable recoil
rescattering. It defaults to off and is now separately configurable; see
[rescattering controls and limitations](validation-recoil-controls.md).

## Semantics

With recoils off, collision finalization assigns recoil status 13 and does not
insert the recoil into the outgoing parton's colour chain. This is the existing
port of the source DOKINEMATICS ALLHAD branch. The runner also applies
GENEVENT's residual status-3 to status-22 exclusion before hadronization and
passes the same flag into the HADROTYPE=0 string adapter.

Simply deleting status-3 particles from a recoil-on colour chain would not
implement the same behavior. The switch applies during generation and colour
construction. It does not perform thermal four-momentum subtraction. Thermal
reference records remain in the diagnostic event output.

The run header records `keep_recoils`; reference replay fixtures carry the
same ALLHAD flag. Analysis treats a missing flag in older native samples as on.
The analysis/report tools now identify the setting per run, and the event audit
rejects excluded recoil statuses in recoil-off hadronization inputs.

```sh
build-pythia8/jewel-run-events norecoil.jsonl 20 2601001 medium replay-inputs \
  --centrality 0 10 --table-energy jewel --longitudinal-boost on \
  --keep-recoils off
ctest --test-dir build-pythia8 -R '^runner-recoils-contracts$' --output-on-failure
ctest --test-dir build -R '^collision-step-differential$' --output-on-failure
```

## Validation

[Machine-readable evidence](validation-recoil-retention.json) preserves the
reference results, run audit and executable hashes.

- The collision-step differential suite passes, including ALLHAD on/off,
  colour assignment and recoil-storage combinations.
- The new runner integration test passes: omitted and explicit on are identical;
  excluded recoils carry no colour, never enter the string input, and the
  off-mode particle momenta/statuses and RNG cursor reproduce the C++ replay
  exactly. Invalid/repeated options are rejected before output creation.
- The 20-attempt recoil-off sample accepts and hadronizes 18 events. The two
  cascade failures are the same attempts as in the recoil-on sample. All 20
  outcomes and cascade draw counts agree; all accepted cascade particle momenta
  are exactly unchanged. Recoil status/colour and string input membership change
  as intended. Accounting, geometry, colourless final-state and hadronization
  conservation audits pass.
- Fortran replay attempt 1 has identical counts but 11 scalar mismatches beyond
  the strict tolerance (max abs 3.32001e-12). Attempt 2 passes (max abs
  4.62685e-14). These are the same strict outcomes as for recoil-on; neither
  the failed comparison nor the failed events are counted as parity successes.

The [native smoke browser](native-recoil-smoke.html), backed by
[native-recoil-smoke.json](native-recoil-smoke.json), was generated using
hepyy/cppyy FastJet. It has 12 selected jets with recoils and 8 without, from
20 attempts each. It is only an integration illustration, far below the required
1000 selected jets. Its JavaScript passes syntax checking; it has not received
a visual browser review. No Fortran spectrum ratio or 10% physics gate is
claimed for these samples.
