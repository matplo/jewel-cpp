# Expanding-medium table energy

Recorded 2026-11-21. Both native runners accept `--table-energy GEV|jewel`.
The default remains 100 GeV for reproducibility. `jewel` uses INIT's EOVEST:

```
PTMAX > 0: min(1.5 * (PTMAX + 50) * cosh(ETAMAX), sqrt(s)/2)
otherwise: sqrt(s)/2
```

The runner currently has PTMAX=100 GeV, sqrt(s)=5020 GeV and ETAMAX mapped
to its medium max_rapidity=2. The resulting scale is 846.494030493817 GeV.
This implements the source prescription for these inputs; it does not select
all stock JEWEL settings. Hard cuts and rapidity are not newly configurable.

The scale feeds initial Sudakov, final splitting, partonic PDF and scattering
rate tables. Event-local geometry updates retain the initialized tables.
Run headers record `table_energy_gev` and `table_energy_prescription`:
`legacy-100`, `explicit` or `jewel-eovest`. Replay exports carry the same scale
in FCONFIG, RCONFIG, XCONFIG and PCONFIG.

```sh
build-pythia8/jewel-run-events medium.jsonl 20 2601001 medium replay-inputs \
  --centrality 0 10 --table-energy jewel
ctest --test-dir build \
  -R '(event-geometry|radiation|final-splitting|pdf)-differential' \
  --output-on-failure
```

## Reference validation

The frozen JEWEL 2.6.0 source and executable hashes, tolerances and full
results are in [the machine-readable report](validation-table-energy.json).
The tests now include the derived scale in each affected table family.

- Initial radiation/Sudakov: 25,590 values, exact equality.
- Final splitting tables and queries: 9,007,224 values, exact equality.
- Partonic PDF tables and queries: 26,420 values, exact equality.
- Event geometry/rates: 3,030 values at table energies 100, 846.494 and
  2510 GeV; maximum absolute error 1.42109e-14, within abs=2e-13,
  rel=2e-12. Each engine's scattering grid remains byte-identical across
  event-local geometry changes.

These counts include the prior settings as well as the new scale.

## Native sample and cascade limits

A 20-attempt 0–10% medium sample at the derived energy accepted 20 events,
with no generation or cascade/hadronization failures. Accounting, geometry,
record integrity and conservation audits pass. All 20 replay fixtures carry
exactly the selected energy in every table configuration.

Two identical initialized states were replayed through C++ and guarded Fortran:

- Attempt 1: both produce 190 records, 67 centres and 98,437 random draws.
  **Strict comparison fails:** 62 scalar differences outside tolerance;
  maximum absolute difference 9.05631e-12.
- Attempt 2: both produce 66 records, 19 centres and 8,862 random draws.
  Strict comparison passes; maximum absolute difference 1.95399e-14.

The unchanged-default 20-attempt fixed-b sample exactly reproduces every
non-header record of the preceding baseline (18 accepted). Invalid numeric
scale inputs (NaN, infinity, nonpositive and trailing text) are rejected before
output creation.

The small new sample is an integration check, not evidence that the known
failure rate is eliminated. No new full-generator jet-spectrum or substructure
ratio gate has been evaluated here; the earlier cascade divergences and
configuration gaps remain open.
