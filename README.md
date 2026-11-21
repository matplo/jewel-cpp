# jewel-cpp

An experimental standalone C++17 port of JEWEL 2.6.0, starting with a frozen-source numerical reference harness. This is not an official JEWEL release or a working event generator. GPL-2.0-or-later; see NOTICE, LICENSE and GUIDELINES.

## Build and validate

The core and probe executable need only CMake and a C++17 compiler. Python and gfortran are needed only for reference validation, not the C++ executable.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DJEWEL_REFERENCE_SOURCE=/path/to/jewel-2.6.0-source
cmake --build build -j 4
ctest --test-dir build --output-on-failure
cat build/validation.json
```

The reference directory must contain `jewel-2.6.0.f.stock` (preferred) or `jewel-2.6.0.f` with the exact hash in `reference/source.json`. Configuration rejects patched or unknown source. Only the selected unchanged routine bodies are extracted into the build directory; the reference executable does not need Pythia6 or LHAPDF. Do not enable fast-math or change Fortran's default REAL size. Omit `JEWEL_REFERENCE_SOURCE` for a C++-only build; that build runs contract tests but makes no reference-parity claim.

```sh
printf 'PQQ 3 0.5 0 0\nALPHAS 5 100 0.2 0\n' | build/jewel-probe
```

Rows contain `operation nf a b c`; output is `value random_draw_count`. For GETT, a/b/c are minimum transfer, maximum transfer, Debye mass; follow the row with a tape length on its own line and tape values on the next line. Shower and scattering Lambda default to 0.2 GeV in the probe. The library exposes separate values.

## Implemented milestone 1a

- Owned event storage with stable particle IDs, parent references, production coordinates, momentum and colour fields. It is an initial append-only graph; colour closure and evolution accounting are not implemented.
- Explicit RNG interface and finite replay tape, checkpoint/restore, exhausted-tape errors. No claim that a standard C++ RNG reproduces PYR; callers supply a recorded tape.
- Medium interface, zero-density vacuum implementation and expanding optical medium (see milestone 1b below). Thermal-scatterer sampling is covered by milestone 1c below.
- Direct translations of PQQ, PGQ, PGG, PQG, ALPHAS, INTPQQ, INTPGGLOW, INTPGGHIGH and GETT. GETT rejection sampling uses the same draw order and empty-interval convention.
- ALPHAS deliberately retains the Fortran single-precision beta-function coefficient before double-precision evaluation. Changing that is a separate numerical change.

The differential suite covers flavours 0/3/4/5/6, logarithmic scales, near-endpoint splitting fractions, near-Landau-pole values on the supported side, transfer intervals, forced rejection, and deterministic random tapes. Every implemented physics kernel is compared against the stock routine with absolute tolerance 2e-13 and relative tolerance 2e-12. GETT draw counts must match exactly. C++ domain-contract tests reject singular endpoints, nonfinite inputs, invalid scales and exhausted tapes; these are deliberate exceptions instead of relying on Fortran NaNs or hangs. No parity claim is made outside the documented physical domain.

## Reference harness and next gates

`reference/freeze.py` records and copies explicitly supplied source, executables, cards, patches and PDF files into a new baseline directory with SHA256 hashes. Label each baseline `stock`, `custom` or `perf`; use separate directories. Supply all relevant PDF members and generator inputs, not just a card that refers to mutable files. This tool freezes inputs; it does not certify sample completeness.

```sh
python3 reference/freeze.py --variant stock --output validation-output/stock-001 \
  --file /path/to/jewel-binary --file /path/to/params.dat \
  --file /path/to/medium.params.dat --file /path/to/pdf-member.dat
```

The full stages 0 and 1 remain open. Next implement the remaining splitting/Sudakov/scattering grids, the run configuration and evolution state machine. Then add controlled initial-state snapshots and evolution replay. Before full-generator comparisons, capture stock and custom run manifests, complete PDF provenance, seeds, event weights, cross sections and rejected-event accounting; build a statistically adequate weighted baseline. Kernel parity is not validation of a full event generator.

Pythia8 hard-event import and separate hadronization adapters follow core evolution validation. No Pythia8, TGlauberMC or JETSCAPE dependency is introduced here. The full routine inventory and ordered port plan are preserved in `docs/port-plan.md` and `docs/routines.csv`.

## Milestone 1b: expanding medium and numerical tables

`include/jewel/medium.hpp` exposes `ExpandingMedium`, `MediumConfig` and
`ExponentialIntegral`. Tables belong to each immutable model; there is no shared
Fortran state. The expanding medium implements CALCTA, NTHICK, NPART, GETTEMP,
GETMD, GETMS, GETNEFF and CALCXSECTION. The numerical layer translates ODEINT,
RKSTEPPER, RK4STEP, EIXINT/EIX/EI, INTPQGLOW/HIGH and SCATPRIMFUNC.

```cpp
#include "jewel/medium.hpp"
jewel::MediumConfig config;
config.impact_parameter = 7.0; // fm
jewel::ExpandingMedium medium(config);
auto local = medium.sample({2.0, 1.0, 0.0, 1.0}); // x,y,z,t in fm (c=1)
auto cross_sections = medium.cross_section_table();
jewel::ExponentialIntegral ei;
auto primitive = ei.scatprimfunc(10.0, 0.5, jewel::Kernels{});
```

Temperature and masses are in GeV; effective density is the stock GETNEFF value
(in natural-unit GeV^3, including its longitudinal boost factor), not fm^-3.
The cross-section table retains the stock three columns and normalization; the
last column is its cumulative weight, not a normalized centrality probability.
It is computed on request. The 100-point thickness table is built at construction.
The Ei table has 1,000 positive/negative nodes and uses stock interpolation.

The `medium-differential` CTest compares every generated table entry, interpolation,
participant density, medium samples and the three Ei-dependent kernels against
unchanged, hash-verified stock Fortran routines. The reference harness dispatches
only DERIV modes 5 and 101 and sets parameters explicitly rather than invoking
MEDINIT/card parsing. Source identities are in `reference/medium-source.json`.
`build/medium-validation.json` records errors and hashes. See
[recorded validation](docs/validation-medium.md).

### Preserved reference details and explicit domain policies

- Preserve default REAL rounding in the nuclear-radius formula, integration
  constants, cooling exponent `0.3333`, and `sqrt(2.)` scatterer mass divisor.
- Preserve the thickness grid's actual last radius (99/100 of `2R`), linear
  interpolation, central participant normalization and strict `T < Tc` cutoff.
- Preserve the Debye floor even when temperature/density vanish. A nonzero
  screening mass alone therefore does not mean the point contains active medium.
- Define temperature and density as zero on/outside the forward light cone;
  stock GETNEFF is undefined there. This is a documented extension, tested as
  a C++ contract rather than included in the Fortran parity claim.
- Ei preserves extrapolation below the first grid knot, including the finite
  table value at zero. This is the stock approximation, not mathematical Ei(0).
  Beyond `|x|=55` C++ throws; stock warns and extrapolates. Dependent kernels also
  throw if they request such a value. Integration failures throw instead of
  printing a warning and returning a partial result.

Complete shower/scattering rate grids and transport evolution remain unimplemented.
It is validation of medium fields and these tables, not full JEWEL event parity.

## Milestone 1c: medium sampling and bounds

`GeometrySampler` builds from a medium's cross-section table and returns impact
parameter plus centrality **fraction** (input centrality limits are percentages).
Use `medium.with_impact_parameter(b)` to obtain event-specific geometry without
rebuilding the thickness table. `sample_vertex(random)` follows stock PICKVTX,
including the stock 10 fm sampling radius; this differs from the Woods-Saxon
radius used to construct the thickness table.

`sample_scatterer(position, random, boost)` follows stock GETSCATTERER species
selection, rejection weights, momentum cutoff, angles and longitudinal boost.
`type=2` is the stock quark-class indicator, not a resolved quark flavour.
The output mass and momentum are in GeV. Stock's zero-momentum cold-medium
fallback is preserved with `synthetic=true` so the evolution engine can diagnose
it. Timelike sampling positions are required. Random draws must be in (0,1),
and exhausted replay tapes throw. The stock species-dependent weight signs
are preserved rather than replaced with a different thermal prescription.

`bounds()`, `average_scatterer()` and `maximum_scatterer()` cover the medium
queries needed by rate tabulation. They preserve the stock distinction between
GETMS's single-precision sqrt(2.) and GETMSMAX's double-precision sqrt(2.D0).
Bounds require positive Tc and Debye scale to avoid undefined divisions.
Centrality input requires 0 <= min < max <= 100; a rounded draw at unity or an
empty cumulative interval throws rather than reading past the stock table.

See [sampling validation](docs/validation-sampling.md). These interfaces still
need integration with the complete shower and run/card configuration.

## Milestone 1d: initial-state radiation integrals and Sudakov tables

`include/jewel/radiation.hpp` provides `InitialRadiation` and
`InitialSudakovTable`, implementing GETINSPLITI, GETINSUDAKOV, INSUDAINT,
GETINSUDARED and GETINSUDAFAST. Channels are `QQ`, `QG`, `GG`, `GC` and `GQ`;
the four stored Sudakov channels exclude `GQ`, as in stock JEWEL.
The table constructor takes Q0, EMAX, SCALEFACM and the medium's maximum
scatterer. It retains all 1,000 nodes, the stock grid endpoints, linear
interpolation and clamping rules. Empty/reversed Sudakov intervals return one.

The constructor requires Q0 > 2 Lambda_shower, and nonfinite/invalid arguments
throw. Ratio evaluation throws on a zero denominator rather than producing NaN.
High-energy initial-radiation integrals explicitly use `ei.extrapolated(x)` to
retain stock Ei extrapolation beyond |x|=55. The bounded `ei(x)` API still throws
outside that interval. Extrapolation is a reference approximation, not an exact
special-function evaluation.

[Validation details](docs/validation-radiation.md) include full tables for four
configurations, including EMAX=6,500 GeV. Final-state splitting tables are covered below; partonic PDF/rate tables and
complete evolution remain to be integrated.

## Milestone 1e: final-state splitting and Sudakov integration

`FinalSplittingTable` implements the full SPLITFNCINT grid (three 1,000 × 1,000
arrays), GETSPLITI interpolation and final-state GETSUDAKOV integration. It
preserves the logarithmic fraction grid, linear scale grid, kinematic cutoff,
per-channel cap at 10, flavour weighting, angular-ordering branch and table
extrapolation. Unsupported/undefined kinematics throw instead of propagating
NaNs. The combined GC channel retains a separate rounding step for the QG
contribution, required to reproduce adaptive integration.

```cpp
jewel::Kernels kernels;
jewel::ExpandingMedium medium;
jewel::FinalSplittingTable final_state(
    kernels, 1.0, 200.0, 1.0, medium.maximum_scatterer(false));
double probability = final_state.sudakov(
    10.0, 100.0, 1.0, 0.5, 100.0, jewel::Channel::QQ, 1);
```

`final-splitting-differential` compares all six million grid values across two
configurations plus grid coordinates and 816 queries. Run it with
`ctest --test-dir build -L full-tables --output-on-failure`. It currently takes
about 22 seconds on the development machine. [Validation report](docs/validation-final-splitting.md).

The full end-state requirements, including all custom-JEWEL features and the
1,000-jet/10% comparison target, are tracked in [completion gates](docs/completion-gates.md).

## Milestone 1f: parton-in-parton PDF tables

`PartonicPdf` implements GETPDF, GETPDFXINTEXACT, PDFINT and GETPDFXINT.
These are JEWEL's partonic evolution densities, not LHAPDF beam distributions.
It owns the associated initial Sudakov model and builds all four 1,000-node
integral tables. Columns are Q², QQ, GQ, QG and GG. Lookup is linear in Q²
with the stock logarithmic bin selection, including extrapolation.

`density(x, q, channel)` preserves the stock endpoint thresholds and returns
zero for finite x outside [0,1] or q below Q0. The x=0 QQ/QG limits remain
available; gluon endpoint singularities return zero as in stock. Nonfinite
input and invalid channels throw. Direct integrated queries require q >= Q0.
If a requested density integrates outside the valid Sudakov support and hits
a zero denominator, C++ throws; the corresponding stock probe produces NaN.
That case is tested and documented, not counted as numerical agreement.

[PDF validation](docs/validation-pdf.md) covers four complete table sets,
density endpoints, direct integrals and extrapolated table queries. Remaining
work includes scattering-rate grids and their integration into evolution.

## Milestone 1g: scattering-rate grids and cross sections

`include/jewel/scattering.hpp` provides `ScatteringRates`, translating XSECINT,
GETXSECINT and GETSSCAT. It builds all four 1,000 × 100 grids (QA/QB/GA/GB),
with logarithmic transfer and linear Debye-mass coordinates. The model owns
its medium, partonic PDFs and Sudakov data, avoiding shared mutable COMMON
state. `integral(t, md, channel)` retains stock bilinear interpolation and
extrapolation; a constant Debye grid uses its single physical column.

`cross_section(projectile, mass, lower_transfer, incoming, outgoing, position,
mode)` supports Q/G projectiles, Q/G/combined-C outgoing channels and all three
query modes: local medium (0), minimum screening (1), maximum screening (2).
The lower-transfer argument is squared internally, as LW in stock GETSSCAT.
It returns the stock integrated cross section; the collision rate still needs
the medium-density and evolution-time factors. Do not treat this call alone as
an event transport algorithm.

The implementation deliberately preserves the reference's use of Lambda_shower
in the second rate integrals and Lambda_scattering in SCATPRIMFUNC, and the
single-precision quark colour factor 2./3. Empty phase space returns zero;
negative accumulated cross sections are clamped to zero as in stock. Invalid
query types or nonfinite inputs throw.

[Scattering validation](docs/validation-scattering.md) compares every grid entry
and channel queries, including distinct coupling scales, medium boosts,
non-unit SCALEFACM and a constant Debye-mass grid. Full transport evolution,
colour/recoil bookkeeping and Pythia8 event integration remain outstanding.

## Milestone 2a: branch candidate sampling

`include/jewel/branching.hpp` provides `BranchingSampler`, translating GETMASS
and GENERATEZ. It retains the Sudakov-overestimate veto loop, kinematic limits,
quark/gluon branch decisions, running coupling, mode-3/4 angular vetoes and exact
random draw ordering. `mass()` returns the candidate mass plus optional splitting
fraction and quark-pair choice; accepted initial-state candidates do not assign
these outputs in stock, so C++ represents them as absent rather than stale data.
Empty phase space and the narrow-interval special case retain stock assignments.

`fraction()` supports QQ, GG and QG and preserves the otherwise unused initial
random draw in the QG branch. Epsilon > 0.5 returns 0.5 without consuming draws.
Invalid square-root domains and zero epsilon throw. Replay exhaustion also
throws, allowing a failed or unexpectedly long rejection loop to be diagnosed.

All ANGORD values 0–5 are covered by replay tests. Initial-state GETMASS calls
in stock GETNEWMASS pass theta=-1. For INS=true, positive theta and modes 3/4,
the reference reads an uninitialized local Z; C++ rejects this undefined API
combination. With the stock theta=-1 convention C++ skips that unused angular
expression and reproduces the observable result and draw sequence.

[Branching validation](docs/validation-branching.md) covers 3,168 replay cases.
This implements branch candidates, not yet the event-tree update, transport
scheduler, colour assignment or complete shower evolution.
