# Sampled event geometry with initialized tables

Recorded 2026-11-21. The native parton and hadronized runners now accept
`--centrality MIN MAX`, in percent. Without this option the existing fixed b=0
behavior is preserved.

```sh
build-pythia8/jewel-run-events central.jsonl 20 2601001 medium \
  --centrality 0 10 replay-inputs
```

The interval must satisfy 0 <= MIN < MAX <= 100. This option can be combined
with external-PDF and mapped-fragmentation settings. It uses the existing
optical-medium cross-section inversion and production-vertex rejection sampler;
it is not a TGlauberMC implementation.

## Initialization versus event geometry

Stock INIT calls MEDNEXTEVT before constructing numerical tables. GENEVENT then
calls MEDNEXTEVT, PICKVTX and GETLTIMEMAX for each event. The port preserves this
separation:

1. A dedicated SplitMix stream, seed `run_seed + 500000`, samples the geometry
   used to initialize the Sudakov, PDF, splitting and scattering tables.
2. The existing per-attempt stream samples the event centrality/impact parameter,
   then the production vertex and subsequent evolution draws.
3. Event-local medium temperature/density, scatterer sampling, cross-section
   bounds, production points and lifetime use that same event impact parameter.
   Initialized numerical tables are not rebuilt for each event.

These independent native random streams are explicit reproducibility choices;
they do not recreate the original global Pythia6 RNG history. Hard-event creation
still occurs before native event-geometry sampling, so failed hard-input attempts
have no geometry record.

`ScatteringRates::with_event_impact_parameter()` retains the initialized grids
while copying the event-local medium. `TransferSampler` has the same explicit
geometry operation. Each event's evolution objects reference the event-specific
models for their full lifetime; they cannot accidentally use the initialization
medium for local scattering. Rate objects currently own a copied grid per event;
sharing immutable table storage is a possible later optimization.

Run headers record `centrality_percent`, `geometry_seed`,
`table_impact_parameter_fm` and `table_lifetime_fm`. Event records contain
`impact_parameter_fm`, `centrality_fraction` (0–1, not percent), and
`event_lifetime_fm`. Processing failures retain the sampled impact parameter and
centrality. The previous development header's `lifetime_fm` is now explicitly
named `table_lifetime_fm`; consumers should use the per-event lifetime for
sampled geometry. Fixed b=0 records have null centrality.

## Validation

The new EVENTB reference-probe command changes BREAL without rebuilding tables.
The corresponding native command changes the medium in both the rate and
transfer models. Differential tests cover the full cross product of two initialization geometries and boost
off/on, five subsequent geometry choices, local temperatures/densities,
bounds/lifetimes and quark/gluon cross sections in local/minimum/maximum modes.
The updated matrix includes table energies 100, 846.494030493817 and 2510 GeV,
and spacetime rapidities −1.5, 0 and +1.5 at each sampled position/proper time.
The nonzero rapidities exercise the longitudinal-flow setting after geometry
changes; z=0 alone would not distinguish flow on/off. Each geometry/flow
combination runs at all three table energies (12 configurations). Proper times
0.2, 0.4, 1, 4, 20 and 100 fm cover early heating, the initial-time transition,
cooling and late shutoff. Zero versus nonzero agreement is required in addition
to the numerical tolerance, preventing an inactive-medium discrepancy from
being hidden by the absolute tolerance.

The extended matrix includes transverse points (0,0), (3,0), (7,0),
(0,3), (3,4) and (−3,−4) fm. Each point is tested with massless incoming
four-momenta (px,py,pz,E) = (10,0,0,10), (6,0,8,10) and
(0,6,−8,10) GeV. This exercises off-axis geometry and partons moving with
and against longitudinal flow without changing the input energy.

**143,220 values pass** at abs=2e-13, rel=2e-12; maximum absolute difference is
1.42109e-14. Each engine's complete scattering grid is byte-identical before and
after all event-geometry changes (24 checks across the two engines). All 11 related
medium, sampling, radiation, final-splitting, PDF and scattering differential
and contract tests pass. This extension changes validation only, not production
physics. [Recorded component results](validation-event-geometry.json).

The fixed-b smoke rerun preserves every pre-existing field of all 20 outcomes
exactly, including momenta, ancestry, random draws and hadronization products.
The new 0–10% run accepts 19/20 events; one cascade fails. Its initialization b is
1.7620092319197471 fm and initialization lifetime is 62.4582300264562 fm.
All sampled centralities lie in the requested interval and agree with their
replay fixtures. This is a changed physics setting, not evidence that cascade
failure handling improved. [Sample audit](validation-native-geometry.json).

Two native events were replayed against unchanged Fortran branch routines with
the initialized tables frozen and the event impact parameter applied afterward:

| Attempt | Result |
|---|---|
| 1 | Same 190 records, 67 stored centres and 98,437 draws; 34 scalar mismatches, max absolute difference 5.03242e-12 |
| 2 | Strict comparison passes; 66 records, 19 stored centres and 8,862 draws; max absolute difference 1.9540e-14 |

Both C++ probe replays reproduce the actual native runner's momenta and draw
counts exactly. The first remains a **failed strict comparison**, with no
changed tolerance. [Replay evidence](native-geometry-replay.json).

```sh
ctest --test-dir build -R 'event-geometry|sampling-differential|scattering-contracts' \
  --output-on-failure
python3 tools/replay_native_cascade.py build/jewel-medium-probe \
  build/jewel-medium-reference replay-inputs replay-results --attempts 1 2
```

## Remaining full-generator gaps

This connects sampled geometry; it does not establish the final jet comparison.
The reduced runner defaults to a 100 GeV table energy scale and now supports
`--table-energy jewel` to derive EOVEST from PTMAX, ETAMAX and SQRTS;
see [table-energy validation](validation-table-energy.md). A matched run
configuration must make that choice explicit. ISR, other channels, remaining tune differences,
custom outputs and known cascade discrepancies remain open. Earlier large native
samples retain their original fixed-b settings and are not relabelled as
centrality-sampled results.
