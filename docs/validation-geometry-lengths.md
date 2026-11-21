# Native hard-leg path-length estimates

The native parton and hadronized runners now accept `--geometry-step FM`.
For example:

```sh
build-pythia8/jewel-run-events medium.jsonl 20 2601001 medium \
  --centrality 0 10 --table-energy jewel --geometry-step .02
```

This opts into the custom JEWEL `geometry-v1` calculation. It makes no random
draws and uses the hard legs after initial kinematic reshuffling, before cascade
evolution. Default output is unchanged. Accepted events receive two `geometry`
records; failed attempts do not receive estimates. The header records the step,
support radius and estimator definition.

Each record identifies its `leg_id` (1 or 2) and `shower_start_id`. Join that ID
to the event's `initial_hard` snapshot for its PDG and shower-start four-momentum;
do not use a compressed final-state array index. The production point is
`[x0,y0,0,0]` in fm, matching the custom estimator's origin convention.

| Field | Definition |
|---|---|
| `L_geom_T_fm` | Transverse distance to the first exit from the intersection of the two finite NTHICK support discs. |
| `L_hot_fm` | Spatial distance along the straight ray during intervals with GETTEMP > 0. |
| `L_hot_T_fm` | Transverse component of that hot-medium distance. |
| `int_T3_dl` | Integral of T cubed over hot spatial distance, in GeV cubed fm. |

The ray velocity is shower-start p/E. Midpoint integration uses a maximum
coordinate-time step in fm/c and stops at the earlier of the lifetime bound
and the transverse exit. It does not stop at the first cold interval because
the medium initially heats. The custom zero-radius convention produces zero
lengths in vacuum. A purely longitudinal leg has zero transverse geometric
length and integrates up to the lifetime bound. Geometric length and hot length
are distinct; rapidity support and cooling can give zero hot length even with
a nonzero geometric distance.

These are estimates for an initial hard direction, **not the distance actually
traversed by a final jet or all its shower branches**. Final-jet association
belongs in analysis after full-phase-space clustering/matching and before
analysis acceptance cuts. This change adds development JSON fields, not the
Fortran CSV/HepMC sidecar interface or a new hepyy-utils importer.

## Reference and tests

`src/geometry.cpp` ports `JG_LENGTHS` from the merged custom recipe. The exact
Fortran routine is retained in `reference/geometry_lengths.f`; only its error
handler is supplied by the harness. The differential test verifies its hash
before running. GETTEMP comes from the existing hash-verified JEWEL 2.6.0
medium reference. This distinguishes custom diagnostic parity from upstream
physics parity.

All **3,264 scalar values match exactly** on this build. The 816 queries cross
impact parameters 0/3/7 fm, Woods-Saxon/flat profiles, central/displaced/boundary
origins, transverse/mixed/longitudinal/stationary motion, integration steps
0.02/0.1/0.007 fm, and lifetimes 0.2/1/30 fm. Vacuum zero-radius cases are also
included. Acceptance retains abs=2e-13 and rel=2e-12, with exact zero/nonzero
agreement. [Source and executable evidence](validation-geometry-lengths.json).

Analytic C++ tests check constant-temperature lengths/integrals, delayed heating,
vacuum/stationary legs and invalid-domain rejection. The medium differential
suite also passes after adding the port.

A native medium event with seed 2601001 gives exact agreement with Fortran for
both exported hard legs. Every pre-existing output field agrees exactly with
the same run without geometry, including momenta, ancestry, random draws and
summary statistics. Seven malformed geometry options are rejected before output
creation. [Native integration evidence](validation-geometry-native.json).

```sh
ctest --test-dir build -R '^(geometry-contracts|geometry-lengths-differential|medium-differential)$' --output-on-failure
python3 tests/geometry_native.py build-pythia8/jewel-run-partons \
  build/jewel-medium-reference geometry-native-validation.json
```

These checks do not establish the full-generator jet-ratio requirement.
