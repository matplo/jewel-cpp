# Expanding medium and table validation

Updated on 2026-11-21, macOS arm64, Release build with AppleClang 21 and
GNU Fortran 16.2. The medium differential suite passes **24,712 scalar
comparisons** against unchanged, hash-verified JEWEL 2.6.0 routines.
[Machine-readable results](validation-medium.json) include source and executable
hashes, input hash, tolerances and all comparison groups.

| Comparison | Values | Maximum absolute error |
|---|---:|---:|
| ei-table | 3,000 | 0 |
| ei-knot-neighbours | 5,998 | 0 |
| ei-interpolation | 18 | 0 |
| LOW | 300 | 2.0817e-17 |
| HIGH | 300 | 2.0817e-17 |
| SCAT | 75 | 0 |
| thickness-table | 800 | 0 |
| thickness-knot-neighbours | 1,192 | 0 |
| cross-section-table | 2,400 | 0 |
| medium-bounds | 28 | 2.8422e-14 |
| maximum-scatterer | 40 | 0 |
| average-scatterer | 200 | 0 |
| thickness-boundary | 24 | 0 |
| participants | 160 | 0 |
| medium-sample | 3,440 | 0 |
| temperature-transition | 180 | 0 |
| overlap-transition | 160 | 0 |
| rapidity-transition | 160 | 0 |
| varied-optical-thickness | 600 | 0 |
| varied-optical-cross-section | 1,800 | 0 |
| varied-optical-bounds | 21 | 0 |
| varied-optical-medium | 240 | 0 |
| screening-bounds | 56 | 1.4211e-14 |
| screening-transition | 960 | 0 |
| varied-rapidity-transition | 2,560 | 0 |

Acceptance uses relative tolerance 2e-12 and absolute tolerance 2e-13.
Temperature and density must additionally agree exactly on zero versus nonzero:
a cutoff disagreement cannot pass using the absolute tolerance.

Every entry of the 1,000-node Ei, 100-node nuclear-thickness and 200-row
cross-section tables is compared. The boundary extension queries both signs of
every Ei knot and adjacent representable doubles within the supported range,
and every thickness knot and its adjacent doubles. Zero is included; the text
probe cannot read subnormal decimal inputs, so the adjacent subnormal at zero
is excluded. Table coordinates come from the reference; table values are
independently calculated and compared.

Medium configurations cover A=208,16,197; central/displaced collisions;
Woods-Saxon and flat profiles; NF=0,3,5; varied initial temperatures and times.
Samples include growth, cooling, overlap edges, late cutoff and rapidity limits.
Three flat-profile configurations additionally isolate the early heating cutoff,
initial-time transition and late cooling cutoff. Each is tested at its calculated
transition, adjacent doubles and relative offsets of ±1e-10. The cooling exponent
uses stock default-REAL rounding of 0.3333. All 180 transition output values agree
exactly on this platform. Exact numerical agreement is not a cross-platform promise.

The additional overlap checks use a flat temperature profile at the initial time,
so cooling cannot mask the geometric edge. Both signs of the transverse overlap
edge are queried at the boundary, adjacent doubles and relative offsets of
±1e-10. Rapidity checks similarly straddle both ±2 boundaries using identical
finite spacetime coordinates in C++ and Fortran. All 320 additional values match
exactly, including zero/nonzero temperature and density decisions. The light cone
remains a documented C++ domain extension, not a reference-parity test.

The reference probe initializes COMMON data explicitly rather than running
MEDINIT/card parsing. DERIV dispatch is a harness implementation limited to the
original mode-5 exp(-x)/x expression and mode-101 MEDDERIV call. Routine bodies
are extracted without edits; no C++ temperatures or table values are supplied
to the reference.

C++ contracts cover analytic quadrature, invalid parameters and positions,
empty medium, light-cone behavior and out-of-range Ei requests. Deliberate domain
extensions are documented in the README and excluded from reference-parity claims.

Related component validation covers [sampling](validation-sampling.md),
[initial radiation](validation-radiation.md),
[final splitting tables](validation-final-splitting.md),
[partonic PDFs](validation-pdf.md) and
[scattering grids](validation-scattering.md). These numerical comparisons do not
validate a complete event generator or final-state jet observables.

## Related checks rerun with this update

All 15 selected CTest suites passed: medium, sampling, radiation,
final-splitting, PDF, scattering, event-geometry, transport and coherent-evolution
comparisons, plus their selected C++ contract tests. The final-splitting
comparison checks 9,006,000 table values, and the scattering comparison checks
1,203,300 grid values, plus their lookup/integral tests. The expanded nonzero-
rapidity event-geometry test also passes independently: 143,220 values, maximum
absolute error 1.42109e-14, with initialized grids unchanged byte for byte.
No production physics code changed in this update.

An uncommitted global `-ffp-contract=fast` experiment was rejected and removed:
it introduced radiation/PDF/scattering-table and transport comparison failures.
Both reference-enabled and Pythia8 builds were rebuilt with the preceding compiler
settings. Recovering an individual cascade event under a different arithmetic
policy is insufficient evidence to change the numerical implementation. Existing
full-cascade discrepancies remain outside this component-validation claim.

The expanded event-geometry matrix independently crosses both initialization
geometries with both flow settings at all three table energies and includes
proper times 0.2, 0.4, 1, 4, 20 and 100 fm. It also requires exact zero/nonzero
agreement. All 11 related medium/table suites were rerun successfully with this
extension; see [event-geometry evidence](validation-event-geometry.json).

The latest combined geometry/rate extension additionally crosses six transverse
positions with three massless projectile directions, including positive and
negative longitudinal momentum. All 143,220 scalar comparisons pass with maximum
absolute difference 1.42109e-14, and all 24 before/after grid checks remain
byte-identical. The 11 medium/table regression suites also pass. This extends
component coverage; it does not resolve the documented full-cascade mismatches
or establish jet-level agreement.

## Non-default optical geometry and screening

The latest extension adds 3,677 scalar comparisons. Optical configurations
`(A, nuclear density, skin depth, sigma_NN)` are `(16, .14, .4, 4.2)`,
`(197, .19, .65, 7.0)` and `(208, .17, .54, 6.199999809265137)`;
lengths are in fm, density in fm^-3 and cross section in fm^2.
Each engine independently computes all 100 thickness rows and 200 centrality
cross-section rows, its medium bounds and 20 spacetime samples per configuration.

Screening configurations `(floor in GeV, scale)` are `(.45, 1)`, `(.45, .6)`,
`(.9, 1.2)` and `(2, 1)`, for both flat and Woods-Saxon profiles. Queries straddle
candidate heating and cooling times where `3 * scale * T = floor`, with adjacent
doubles and relative offsets of ±1e-10, at spacetime rapidities -1.5, 0 and 1.5.
These cover an active crossover, a crossover hidden by the temperature cutoff,
and a floor above the peak thermal screening mass. The candidate times only
select query coordinates; temperatures and screening masses are independently
computed by the C++ and Fortran routines. All 960 sampled values agree exactly;
zero/nonzero temperature and density agreement is enforced separately.

No production implementation or numerical tolerance changed. This extends
component validation and does not establish full-generator jet agreement.

For this extension, all 13 selected CTest suites passed after rebuilding:
medium, radiation, final-splitting, PDF and scattering contracts and differential
checks, plus sampling, event-geometry and geometry-lengths differential checks.
The full-table checks retained exact agreement for 9,006,000 final-splitting
values and 1,203,300 scattering-grid values. Event geometry/rates retained
143,220 passing comparisons (maximum absolute error 1.42109e-14) with all
24 initialized-grid preservation checks passing.

Reproduce with:

```sh
cmake --build build -j 4
ctest --test-dir build --output-on-failure -j 3 \
  -R '^(medium|sampling|radiation|final-splitting|pdf|scattering|event-geometry|geometry-lengths)-(contracts|differential)$'
```

## Configurable rapidity boundaries

The additional 2,560 scalar comparisons vary the rapidity limit over 0.5, 1,
3 and 5, using both flat and Woods-Saxon profiles and impact parameters 0 and
7 fm. At transverse position (1, 1) fm, proper times 0.2, 0.4, 1 and 30 fm
cover growth, the initial-time transition, cooling and the late cutoff.
Both signed rapidity edges are queried at the edge, adjacent representable
rapidities and offsets of 1e-8. Both engines receive identical Cartesian
spacetime coordinates; rounding in the conversion can move a nominal edge
point across the cutoff, so parity is judged on the resulting coordinates.

All added values agree exactly on this platform. Temperature and density
zero/nonzero decisions are checked independently of the unchanged absolute
and relative tolerances. The report records the rapidity limits and input,
source and executable hashes. No production physics code changed.

After this extension, all 13 selected medium/table CTest suites passed.
The full splitting tables retain exact agreement for 9,006,000 values and
scattering grids for 1,203,300 values. Event geometry/rates pass all 143,220
comparisons with maximum absolute error 1.42109e-14. These remain component
checks, not evidence of full-generator jet-level agreement.
