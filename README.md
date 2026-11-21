# jewel-cpp

An experimental standalone C++17 port of JEWEL 2.6.0, starting with a frozen-source numerical reference harness. This is not an official JEWEL release or a complete, physics-validated event generator. GPL-2.0-or-later; see NOTICE, LICENSE and GUIDELINES.

## Build and validate

The core and probe executable need only CMake and a C++17 compiler. Python and gfortran are needed only for reference validation, not the C++ executable.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DJEWEL_REFERENCE_SOURCE=/path/to/jewel-2.6.0-source
cmake --build build -j 4
ctest --test-dir build --output-on-failure
cat build/validation.json
```

The reference directory must contain `jewel-2.6.0.f.stock` (preferred) or `jewel-2.6.0.f` with the exact hash in `reference/source.json`. Configuration rejects patched or unknown source. Only the selected unchanged routine bodies are extracted into the build directory; the reference executable does not need Pythia6 or LHAPDF. The duplicate WEIGHT declaration in DOINSTATESCAT is normalized exactly as recorded in the reference manifest; executable statements are unchanged. Do not enable fast-math or change Fortran's default REAL size. Omit `JEWEL_REFERENCE_SOURCE` for a C++-only build; that build runs contract tests but makes no reference-parity claim.

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

The full stages 0 and 1 remain open. The numerical grids are implemented below; next implement the run configuration and evolution state machine. Then add controlled initial-state snapshots and evolution replay. Before full-generator comparisons, capture stock and custom run manifests, complete PDF provenance, seeds, event weights, cross sections and rejected-event accounting; build a statistically adequate weighted baseline. Kernel parity is not validation of a full event generator.

Pythia8 hard-event import and a separate hadronization adapter are optional components described below. The core remains independent of Pythia8, TGlauberMC and JETSCAPE. The full routine inventory and ordered port plan are preserved in `docs/port-plan.md` and `docs/routines.csv`.

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
participant density, medium samples, interpolation-knot neighbours, heating/cooling
cutoff transitions and the three Ei-dependent kernels against
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
  Beyond `|x|=55` C++ throws; stock warns and extrapolates. Initial-radiation kernels explicitly use stock extrapolation beyond this range,
  as documented in milestone 1d. Integration failures throw instead of
  printing a warning and returning a partial result.

Shower/scattering rate grids are covered by the later milestones below.
Later milestones implement transport and collision components; these checks do not establish full JEWEL event parity.

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
work includes integration of the scattering-rate grids into evolution.

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


## Milestone 2b: kinematics and owned shower state

`kinematics.hpp` implements rotations, Lorentz boosts, momentum angles, rapidity,
pseudorapidity and GETPOSITION propagation. These preserve the selected
PYANGL/PYROBO/PYP conventions used by JEWEL. Boosts above the reference limit
are rescaled to 1−1e-12 and reported to the caller. Propagation uses max(|p|, E)
as in GETPOSITION. Undefined zero-momentum propagation throws.

`shower_state.hpp` provides stable particle IDs, binary daughter links, colour
allocation metadata, branch variables and validated snapshots for rollback.
Physical production coordinates are stored separately from auxiliary generator
coordinates, so temporary momentum-frame rotations need not change production
points. ISR ancestors may be inserted after their children; ancestry cycles are
rejected. Restoring a snapshot never reuses IDs issued after that snapshot.
This is supporting state, not yet MAKESPLITTING or a complete shower scheduler.

[Kinematics validation](docs/validation-kinematics.md) records 6,083 comparisons.
The reference-only executable additionally requires the hash-verified
`pythia6425mod-lhapdf6.f` supplied with JEWEL. No Pythia6 library is linked into
the standalone C++ implementation. The probes exercise active-particle transforms
and selected momentum observables, not the full Pythia6 API.


## Milestone 2c: collision-time veto sampling

`ScatteringRates::next_collision()` translates GETDELTAT, using the medium and
rate tables owned by the same model. It takes a projectile, production point,
start time, maximum interval, plasma lifetime, cross-section multiplier and
elastic-only switch. It returns whether a collision was accepted, the elapsed
time and the number of rate-bound violations observed. Times are in fm/c.

```cpp
// rates is a ScatteringRates model; random implements jewel::Random.
auto next = rates.next_collision(
    {50., 0., 0., 50.}, 0., 21, {0., 0., 0., 0.},
    0.4, 5., 20., 1., false, random);
// next.collision identifies an accepted candidate at start + next.elapsed.
// Applying its momentum transfer, radiation and recoils is a separate step.
```

The implementation preserves the factor of 5 in the collision rate, velocity
p/E (distinct from GETPOSITION), the lifetime cap, six consecutive empty-medium
trials and exact random-draw ordering. A zero rate or already-expired lifetime
returns false with elapsed=0 as in stock; callers must not interpret that as a
collision or repeatedly schedule the same unchanged state. An ordinary interval
exit returns false with the capped interval. A bound violation is reported but
the acceptance weight is not silently altered. The million-trial limit throws
instead of returning an unfinished stock loop result. Invalid/nonfinite input
and exhausted random tapes also throw.

[Transport sampling validation](docs/validation-transport.md) covers both channel
modes, quark/gluon projectiles, medium exits and replay draw counts. This does not
yet apply collisions or recoils to the event, and is not full transport validation.


## Milestone 2d: angular-rejection repair

`reject_splitting(state, mother, sampler, random)` in `jewel/shower.hpp`
translates REJECTSPLITTING. It resamples sibling masses, repairs their momenta,
updates branch fractions and angles, adjusts decay/evolution times and marks
rejected-scattering descendants inactive. The sampler must use the run's kernel,
cutoff and angular-ordering configuration. Production coordinates remain separate
from auxiliary generator coordinates, which follow the reference rotations.

The state is mutated in place. As in stock, impossible kinematics marks the event
as discarded and returns before restoring the temporary frame. A discarded event
must not be consumed as a valid final state. Exceptions likewise require caller
handling; use an explicit snapshot if rollback is wanted. Undefined zero-axis or
zero-transverse-rescaling inputs throw; they are not included in reference parity.
A one-million-trial guard replaces an otherwise unbounded rejection loop.

[Validation](docs/validation-rejection.md) compares full repaired records and
random consumption, with independent momentum-conservation and mass-shell checks.
This prepares the rejection path needed by MAKESPLITTING; accepted branch
construction and the complete shower scheduler remain to be implemented.


## Milestone 2e: accepted branch construction

`make_splitting(state, particle_id, sampler, medium, formation_factor, random)`
translates MAKESPLITTING. It samples the two daughter masses, retries forbidden
kinematics, applies all six ANGORD modes, invokes sibling repair on angular
rejection, assigns colour connections and creates the two daughter records.
Daughter order follows stock: radiated gluon/antiquark first, continuing parton
second. Gluon-to-quark-pair daughters retain stock's placeholder flavours −1/+1.

Formation times preserve the source's single-precision `0.2` factor. As in the
original routine, daughter birth times are assigned here, while spatial production
coordinates and evolution bookkeeping must be completed by MAKEBRANCH. Newly
owned records initialize these pending fields to zero; they must not be treated
as physical production points until that caller step is implemented. Physical
production points are never rotated with the auxiliary generator coordinates.

Event discard and retry behavior follow the reference, including its temporary
frame on zero-fraction failure and its last-record rotations on impossible
kinematics. Discarded records must not be used for physics output. Invalid input,
undefined kinematics and one million retries raise exceptions; callers own
rollback via snapshots. The formation factor must be finite and positive.

[Branch construction validation](docs/validation-splitting.md) covers full records,
colour flow, formation times, angular rejection and second-generation daughters.
This is a working branch operation, not a complete vacuum or medium shower:
MAKEBRANCH scheduling, scattering/recoil application and Pythia8 integration
remain outstanding.


## Milestone 2f: scattering-induced virtuality updates

`BranchingSampler::updated_mass()` translates GETNEWMASS. `VirtualityUpdate`
contains the new/old spacelike transfer squared, previous candidate mass,
current record mass and energy, PDG code, initial-state switch, energy fraction
and previous branch fraction/pair flag. Supply initial and final Sudakov tables
from the same run configuration as the sampler. The operation returns updated
values without mutating an event; the caller applies them to the selected line.

The update preserves branches when required by the Sudakov ratio, resamples
when the allowed scale changes and caps the resulting mass at the available
energy. Early returns and accepted initial-state samples can retain the prior
fraction/pair flag. Those values are explicitly carried through instead of
being replaced with zeros. The stock default-REAL `1.0000001` threshold is
preserved. Undefined zero-denominator decisions and retry exhaustion throw.

`ScatteringRates::no_initial_radiation()` translates GETPNORAD1 using the owned
medium, average scatterer and rate grids. It preserves the reference quark/gluon
colour factors and raw return convention, including −1 for a gluon with zero
total cross section (versus 1 for a quark). Values are not clipped to [0,1].
Zero scattering invariants preserve the source's finite energy-cutoff or
zero-cross-section return; negative invariants throw.

[Validation](docs/validation-virtuality.md) covers increasing/decreasing scales,
all angular-ordering modes, initial/final updates, retained metadata, different
couplings and medium boosts. Momentum-transfer sampling, coherent scattering,
recoils and the complete evolution scheduler remain to be integrated.


## Milestone 2g: momentum-transfer sampling and accumulation

`jewel/transfer.hpp` provides `TransferSampler`, `TransferRecord` and
`TransferHistory`, translating GETQVEC. The sampler owns its medium and coupling
configuration. `sample()` takes the event, distinct projectile/scratch IDs,
history, zero-based slot, elapsed time since production, energy fraction,
recalculation switch and RNG. The slot corresponds to stock J=NEND, as in every
stock call site. The scratch ID denotes the dedicated temporary record, not a
physical outgoing jet parton.

Fresh sampling draws a thermal scatterer, transforms to its rest frame, samples
transfer t and azimuth, transforms the kick back and accumulates its four-vector.
Recalculation reuses the stored scatterer and transfer limit while resetting the
sum as in stock. Projectile momentum is restored exactly; auxiliary coordinates
retain the reference's transform/rounding behavior. Stored scatterer coordinates
and caller-supplied transfer time are preserved. Input/output units follow the
core: GeV for momenta/masses, GeV² for invariants and fm for positions/times.

A timelike accumulated transfer triggers retries. After the stock 1,002-attempt
limit the stored transfer is zeroed; the scratch record still holds the final
rejected candidate, so physics consumers must use the transfer history. The
10,000-transfer capacity marks the event discarded. Undefined rest frames,
missing recalculation records, nonfinite sums and invalid fractions throw.
Exceptions can leave partial mutations; rollback must cover both event and
transfer history. No automatic history rollback is implied by ShowerSnapshot.

[Validation](docs/validation-transfer.md) checks thermal draws, both longitudinal
directions, accumulated/recalculated transfers and the retry-limit fallback.
This supplies transfer candidates; DOCOHSCATTERING, collision application,
recoil/colour construction and full transport scheduling remain unfinished.


## Milestone 2h: final-state scattering sequence

`ScatteringEvolution::final_state()` in `jewel/evolution.hpp` translates
DOFISTATESCAT. It combines collision-time sampling, momentum-transfer history,
virtuality updates and formation-time accumulation. `EvolutionConfig` supplies
Q0, SCALEFACM, FTFAC, plasma lifetime and the cross-section multiplier; the
referenced numerical models must outlive the evolution object and use matching
settings. This interface operates on an existing first transfer.

`TransferHistory::first/last` carry the one-based active NSTART/NEND range;
allocated records outside that range can retain rejected candidates as in stock.
The return value contains elapsed time, candidate mass, fraction, pair flag and
above-cutoff flag. The same initial lifetime draw is reused through the sequence,
and the final 1/N rejection and collapse-to-first-transfer conventions are
preserved. The source's 100-transfer limit is distinct from GETQVEC's storage
limit. A one-million-iteration guard catches non-converging input.

The operation updates transfer history and scratch records, but does not apply
those kicks to outgoing particles or build recoils. It may retain branch metadata
when restoring the old mass; callers must use the complete returned state.
Rollback must include both the event and transfer history. See
[validation](docs/validation-final-scattering.md) for whole-sequence comparisons.
DOKINEMATICS, initial-state/coherent evolution, complete shower scheduling and
Pythia8 event integration remain outstanding.

## Milestone 2i: recoil storage for collision application

`store_recoil()` in `jewel/recoil.hpp` implements the DOKINEMATICS storage
section for all RECMODE values 0–3. It records incoming centres or momentum
transfers and creates subtraction dummies along the source-prescribed direction,
including negative-energy transfer handling. `RecoilStorage` owns records and
an independent used count; snapshots must include both for rollback.

Soft recoils in modes 0/1 advance the count without replacing that slot, as in
the original. Full storage and rejected/disabled writes leave the count unchanged.
Invalid dummy projections throw; callers must restore or discard partial state.
[Section-level validation](docs/validation-recoil.md) uses unchanged,
hash-verified Fortran statements and compares every allocated slot.
This is a dependency of collision application, not yet a complete DOKINEMATICS
implementation or final-event recoil subtraction.

## Milestone 2j: collision momentum reconstruction

`scatter_pair()` in `jewel/collision.hpp` implements the one-transfer momentum
section of DOKINEMATICS: centre-of-mass or scatterer-rest-frame reconstruction,
KINMODE 0–3, recoil-mass sampling, kinematic retries and temperature-dependent
recoil mass. It returns candidate records and bookkeeping without appending to
the event. Parent/daughter links, colours, timing, recoil storage and the outer
sequence retry still need integration.

[Validation](docs/validation-collision.md) compares 381 finite reference cases
and separately records three cases where Fortran produces NaNs and C++ throws.
The two finite fallback cases with small conservation residuals are retained
and reported. This remains section-level evidence, not full-event validation.

## Milestone 2k: collision-step colours, timing and recoil records

`finish_collision()` completes each accepted momentum pair's recoil
classification, colour assignment, splitting metadata, production/decay times
and subtraction storage. The configuration includes formation-time and recoil
cuts, hadronization/rescattering switches and RECMODE. Timing explicitly carries
the original decay time, coherent end time, next transfer time and current
transfer time. Candidate objects must be restored or discarded on exceptions.

[Combined validation](docs/validation-collision-step.md) covers 750 finite
reference cases and explicitly records 18 nonfinite reference cases. Nearly
lightlike dummy masses have separately reported, roundoff-bounded invariant
comparisons. Event genealogy, multi-transfer sequencing, the final lifetime
overwrite and outer retries are still pending; full DOKINEMATICS parity is not
yet claimed.

## Milestone 2l: complete collision-sequence orchestration

`apply_collisions()` implements full DOKINEMATICS sequence control, including
multiple transfers, event genealogy, final lifetime overwrite, target-mass
fallbacks, shortened retry ranges, colour/count restoration and capacity checks.
The transfer range is zero-based and inclusive; the returned range end,
fraction and retry flag carry the source's updates. Original recoil properties
are retained separately from intermediate outgoing records.

`ShowerParticle::collision_source` preserves recoil ancestry while leaving its
stock primary-parent field zero. Stable event IDs survive snapshots and are
never recycled after rollback. A rejected attempt restores active links and
counts while preserving the source's round-trip arithmetic and recoil-storage
tail. Exceptions require caller rollback/discard; rejected-attempt random draws
are not rewound.

[Full-routine validation](docs/validation-collision-sequence.md) checks 534
wholly finite reference calls and separately records 30 nonfinite reference
cases and 12 guarded/exhausted runs. These are explicit limitations, not successful
collision outputs. This completes the collision-application component; complete
shower evolution and final-event validation are still in progress.

## Milestone 2m: initial-state split construction

`make_initial_splitting()` in `jewel/shower.hpp` implements MAKEINSPLIT, including
all four flavour transitions, colour topology, signed continuing virtuality,
emission-mass fallback, returned X=1 rejection, production points and lifetimes.
It returns stable daughter IDs and the updated fraction; rejected splits append
no records. The caller supplies the physical split request and a sampler with
matching run settings.

[Full-routine validation](docs/validation-initial-splitting.md) checks 781 finite
cases and explicitly records 59 reference NaN cases rejected by C++. This is the
initial-state split constructor; initial-state/coherent evolution and complete
shower scheduling remain outstanding.

## Milestone 2n: coherent-scattering arithmetic

`coherent_resolution()` and `reconstruct_coherent()` in `jewel/coherence.hpp`
implement the resolution gate and daughter-momentum reconstruction used inside
DOCOHSCATTERING. They retain its literal transfer-component ordering and strict
threshold comparison. Reconstruction returns the parent, auxiliary vertex,
possible daughters and kinematic rejection state without mutating an event.

[Section-level validation](docs/validation-coherence.md) compares 1,353 defined
reference cases, with another 109 undefined reference cases explicitly rejected
by C++. Coherent event merging, loop orchestration and rollback remain pending;
these helpers do not yet implement full coherent shower evolution.

## Milestone 2o: coherent initialization and two-input genealogy

`begin_coherent()` implements DOCOHSCATTERING eligibility, sibling ordering,
interval adjustment, saved pre-merge records, angle extraction and the initial
coherent event record. `ShowerParticle::coherent_partner` records its second
incoming leg while retaining the stock primary-parent field. Both incoming
records point to the same intermediate record; snapshots preserve this graph,
and validation follows both ancestry edges.

[Initialization validation](docs/validation-coherent-start.md) checks 330
reference cases and all 36,270 output values. Collision descendants explicitly
clear the second-input field, retaining their immediate origin. The coherent
scattering loop, partial rollback and final resplitting still need integration.

## Milestone 2p: complete coherent evolution

`evolve_coherent()` in `jewel/coherence.hpp` connects initialization, elastic
collision-time sampling, transfer resolution/retries, collision application,
failed-kick rollback, daughter propagation, final resplitting and weighted
coherent histograms. It returns the selected new daughter and saved pre-merge
records. Callers own snapshots of event, recoil, transfer and histogram state
for exceptions or the enclosing shower's separate coherence-reversion policy.

[Full-routine validation](docs/validation-coherent-evolution.md) compares 124
finite JEWEL reference calls and explicitly records seven nonfinite subtraction
cases. Stored transfer momenta have a separately documented numerical tolerance;
discrete fields and random-draw counts agree exactly in the finite cases.
Initial-state evolution is covered below; complete shower/Pythia8 integration remains pending.

## Milestone 2q: initial-state scattering evolution

`InitialScatteringEvolution::sample()` in `jewel/initial_evolution.hpp` implements
DOINSTATESCAT: radiation/flavour selection, transfer sampling and resolution,
virtuality updates, formation-time weighting, coherent evolution/reversion and
retry/fallback bookkeeping. The caller receives the selected leg, fraction,
virtuality, flavour, elapsed time and cutoff state before constructing the split.

[Full-routine validation](docs/validation-initial-evolution.md) checks 52 finite
reference cases, including 659 coherent reversions. Partial rollback retains
source evolution-time slot metadata while preserving stable particle IDs.
The reference needs one documented duplicate-declaration normalization; its
executable statements remain unchanged. Complete shower scheduling and Pythia8
integration remain pending.

## Scheduler integration (in progress)

`Cascade::branch()` and `Cascade::evolve()` in `jewel/cascade.hpp` connect the
ported routines into branch and cascade evolution. Validation is incomplete:
the expanded cascade replay currently fails. See the [development checkpoint](docs/scheduler-progress.md)
for the passing baseline, the first traced numerical divergence, and remaining
work. The reduced native event runner is available below; complete generator validation remains open.

The [interactive cascade comparison](docs/cascade-ensemble-report.html) shows
1,000 attempted showers per engine with coherence on/off, jet spectra, girth,
pTD and failed-event accounting. See the [validation notes](docs/validation-cascade-ensemble.md)
for reproduction and transactional collision retries and remaining discrepancies. This is a
controlled parton-level study, not a passing full-generator comparison.

## Compression boundary routine

`compress_records` ports COMPRESSEVENT, including protected records, moved
genealogy and physical-slot metadata. [Exact reference validation](docs/validation-compression.md)
covers 222 cases and 1,531,146 values. `ShowerState::compress()` now preserves stable-ID ancestry in an archive while
physics follows separate source scheduling links. Enable scheduler integration
with `CascadeConfig::compress_records=true`. Vacuum, threshold and one medium
cascade comparison pass; another medium fixture retains four stored-transfer
discrepancies. Full compression-mode and generator validation remain open.

## Optional native Pythia8 boundary

`Pythia8Input` imports hard-event snapshots and maps hard roles to final
beam-recoil copies through ancestry. `Pythia8Hadronizer` uses a separate
instance for a complete colour-connected input state. A native
`jewel-pythia8-input` executable writes snapshots without Python or Fortran.
[Build instructions and validation](docs/validation-pythia8.md) cover a
1,000-event input/hadronization round trip. Reduced PPJJ shower insertion and hard initialization are now available
through `jewel-run-partons`; `jewel-run-events` also performs HADROTYPE=0
string construction and Pythia8 hadronization. Full-generator validation
remains pending. This is not yet a complete event generator.

## Hard initialization and native parton runner

`initialize_hard_dijet()` implements the PPJJ FINDVIRTUALITIES branch and
GETTHETAM with 288 passing Fortran comparisons. `prepare_pythia_dijet()` maps
Pythia final hard copies into the cascade while retaining frozen beam remnants.
The development `jewel-run-partons` executable produces explicit accepted and
failed-event records, with production coordinates, ancestry, momenta and thermal
references. The separate `jewel-run-events` executable also hadronizes the
evolved state, as described below.

[Validation and reproduction instructions](docs/validation-hard-initialization.md)
record 20/20 accepted vacuum attempts and 16/20 medium attempts. The four medium
failures remain unresolved and are counted explicitly; these smoke tests are
not a passing full-event comparison with JEWEL.

### Native medium replay correction

Exact initialized-state replay exposed and fixed a zero-invariant branch in
GETPNORAD1. The same 20-attempt medium sample now accepts 18 events. The two
remaining failures also produce nonfinite states in guarded Fortran replay;
other cascade mismatches remain open. See [replay evidence and limitations](docs/validation-native-replay.md).

## Native hadronized development runner

`jewel-run-events output.jsonl attempts seed vacuum|medium` connects Pythia8
hard input, JEWEL virtuality initialization and cascade, HADROTYPE=0 string
construction, and a separate Pythia8 hadronizer. It preserves input IDs and
thermal references and records failures without replacing failed attempts.
The output includes the native parton event, hadronization inputs and complete
Pythia8 hadronization history. This is development JSONL, not HepMC.

[Validation](docs/validation-strings.md) covers 292 exact Fortran string-routine
comparisons and small native samples: 20/20 vacuum and 18/20 medium attempts
produce hadronized output. HADROTYPE=1, junction mapping, ISR/MPI and the
full-generator physics comparison remain incomplete.
