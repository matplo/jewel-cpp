# Expanding medium and table validation

Updated on 2026-11-21, macOS arm64, Release build with AppleClang 21 and
GNU Fortran 16.2. The medium differential suite passes **18,475 scalar
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
rapidity event-geometry test also passes independently: 8,430 values, maximum
absolute error 1.42109e-14, with initialized grids unchanged byte for byte.
No production physics code changed in this update.

An uncommitted global `-ffp-contract=fast` experiment was rejected and removed:
it introduced radiation/PDF/scattering-table and transport comparison failures.
Both reference-enabled and Pythia8 builds were rebuilt with the preceding compiler
settings. Recovering an individual cascade event under a different arithmetic
policy is insufficient evidence to change the numerical implementation. Existing
full-cascade discrepancies remain outside this component-validation claim.
