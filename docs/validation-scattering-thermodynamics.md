# Expanding-medium thermodynamics and scattering tables

The expanded harness compares the native C++ implementation with the frozen
JEWEL 2.6.0 Fortran routines. It preserves the three preceding configurations
and adds three combinations of medium parameters and table energy:

| Profile | tau initial (fm) | T initial (GeV) | T cutoff (GeV) | Screening scale | Screening floor (GeV) | Table energy (GeV) | Longitudinal boost |
|---|---:|---:|---:|---:|---:|---:|---|
| Flat | 0.6 | 0.45 | 0.15 | 0.6 | 0.45 | 100 | off |
| Woods-Saxon | 0.3 | 0.70 | 0.12 | 1.2 | 0.45 | 846.494030493817 | on |
| Flat | 0.6 | 0.45 | 0.15 | 1.2 | 0.9 | 2510 | on |

The screening scale multiplies the thermal Debye mass; it is distinct from
the scattering renormalization scale. The detailed report records both, the
flavour count, shower/scattering couplings, source hashes and executable hashes.
This is a selected matrix, not an exhaustive Cartesian scan.

For each of six configurations, both engines independently construct all four
1,000 by 100 scattering grids and both axes. Boundary queries include adjacent
representable values and the source's extrapolation behavior. Rate queries
cover quark/gluon projectiles, all three medium modes, both rapidity signs,
three transverse positions, and proper times tau_initial/2, tau_initial,
2*tau_initial, 10 and 100 fm. These include active and empty medium points.

| Comparison | Values | Maximum absolute difference |
|---|---:|---:|
| Complete grids and axes | 2,406,600 | 0 |
| Integrals and scattering rates | 3,324 | 2.220446049250313e-16 |
| Grid boundaries and extrapolation | 210,132 | 0 |

All **2,620,056 comparisons pass**. Relative tolerance remains 2e-12 and
absolute tolerance 2e-13. Finite outputs and exact zero/nonzero agreement
are required separately. No production physics implementation or tolerance
was changed.

Run from the repository root after building the reference-enabled targets:

```sh
ctest --test-dir build --output-on-failure -R '^scattering-differential$'
```

The test is `tests/scattering_differential.py`; it writes
`build/scattering-validation.json`. The saved run is
[validation-scattering-thermodynamics.json](validation-scattering-thermodynamics.json).

These checks validate medium-dependent numerical tables and local rates.
They do not establish agreement of complete shower histories or final-state
jet distributions; the previously documented cascade and jet discrepancies
remain open.

The 12 related CTest regression suites also passed: medium, radiation,
final-splitting and PDF contracts/differentials, scattering contracts,
sampling differential, event-geometry differential and geometry-lengths
differential. Together with the expanded scattering differential above,
all 13 selected checks passed.
