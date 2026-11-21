# Native legacy MPI beam PDFs

`LegacyMpiPdf` uses LHAPDF's C++ API to reproduce the nucleon branch of the
frozen JEWEL 2.6.0 PYPDFU, followed by PYSIGH charm/bottom suppression.
The native code has no Pythia6 dependency. The validation executable links the
complete unchanged, SHA-256-pinned bundled Pythia6 source against LHAPDF 6.5.5.

The implementation evaluates at x divided by remaining beam momentum, applies
the chosen scale floor before the square root, reconstructs light valence/sea
densities in source order, symmetrizes strange/charm/bottom densities, applies
neutron isospin and antiparticle conjugation, clamps negative densities and caps
flavours. Heavy-flavour thresholds use the original input scale and the source's
default-REAL 1.1 literal. Output order is -5 through -1, gluon, then 1 through 5,
as x*f evaluated at the rescaled fraction.

## Scale-floor audit

The initial differential comparison failed below the CT14nlo minimum scale.
The native metadata policy gives QMin squared = 1.6770249999999998 GeV squared,
while the linked Fortran probe reports VINT(231) = 0. Symbol inspection shows
separate W50513 storage in the macOS executable and LHAPDF dynamic library.
LHAPDF's PDFSET populates that common block in its compatibility interface;
PYPDFU reads it for the floor. The observed split is consistent with the
platform's shared-library binding behavior. This is a build-specific observation,
not a claim that every JEWEL build has a zero floor.

The harness now tests two explicit policies:

- Linked-common: initialize PDFSET through a normal PYPDFU call, read the actual
  Fortran common floor, and supply that floor to the native adapter.
- Metadata: initialize the same reference, explicitly populate its common floor
  through LHAPDF GETQ2MIN, then compare with the native default metadata policy.

The PYPDFU routine body is unchanged in both tests. No existing generator binary
or reference event sample was modified. The metadata test is explicitly a
harness-initialized convention, not the unmodified linked-common behavior.
A native event integration must choose its floor from the reference build audit;
it must not silently assume the two policies are interchangeable.

## Results and limits

[Results](validation-legacy-mpi-pdf.json): 4,368 configurations for each policy,
96,096 scalar comparisons in total, with zero absolute/relative difference on
this platform. Zero/nonzero decisions are checked separately. Cases include
proton, neutron and both antiparticles; remaining-momentum rescaling; endpoints
and small x; low/high scales; adjacent doubles at both heavy thresholds; and
flavour caps 0, 3, 5, 6. Seven invalid query cases and three invalid floor values
are rejected. The same comparison also passes in the
[Pythia8-enabled build](validation-legacy-mpi-pdf-pythia8-build.json).

All eight selected native MPI component differential suites pass. No tolerance
was relaxed. The tested PDF set is CT14nlo/0 (legacy ID 13100), with MSTP52=2,
MSTP57=1. Nuclear PDFs, other beam species, the modern MPI companion model and
arbitrary PDF-set validation remain outside this result. The PDF adapter is not
yet wired into a complete native MPI generator. Jet-level parity remains open.

Configure with `-DJEWEL_LHAPDF_ROOT=/path/to/lhapdf/prefix`, build
`jewel-legacy-mpi-pdf-probe` and `jewel-legacy-mpi-pdf-reference`, then run:

```sh
ctest --test-dir build --output-on-failure -R '^legacy-mpi-pdf-differential$'
```

## Composition with live MPI channels

The [combined replay](validation-legacy-pdf-channel-composition.json) captures
100 initialization and 100 production ISUB=96 calls from a 1,000-attempt reference
run. The read-only instrumentation is saved in
`reference/mpi_pdf_channels_trace.patch`; phase markers use the existing
`reference/mpi_phase_core.patch` and `reference/mpi_phase_trace.f90`.
The resulting HepMC file is byte-identical to the uninstrumented baseline
(SHA-256 64a5289388223a4fb5e1b272007a431c773b5125cb3277e66d6718733e275ca3).

For each call, the native PDF adapter receives the captured beam IDs, original
momentum fractions, remaining beam momentum, PDF scale, flavour cap and heavy
masses. All 4,400 PDF values agree exactly. The replay replaces the captured
PDF arrays with these independently calculated native values before evaluating
channels; it also replaces the captured annihilation width with the native
width calculation using the audited decay configuration.

All 75,360 channel comparisons (raw weights, PDF-weighted weights and totals)
for 37,580 ordered channels agree exactly. Channel identities and counts agree
as well. This passes in both reference-enabled and
[Pythia8-enabled builds](validation-legacy-pdf-channel-composition-pythia8.json).
The older captured-PDF replay still passes after the analysis-tool extension.

The live PDF scales range from 6.121391367869842 to 3250.3115998026565 GeV squared;
remaining beam momentum fractions range from 0.5190760159042944 to 1. These live
scales are above CT14's metadata minimum, so the separate constructed comparison
is necessary to test the low-scale policy difference. The observed reference
floor remains zero and is explicitly supplied to the native adapter.

MINT(31) ranges from 0 through 6 in this trace, while MINT(35) remains 1.
PYPDFU requires a selected beam side AND MINT(35)>=2 for companion reshaping;
a nonzero MINT(31) alone does not activate it. The replay verifies the actual
stage guard and supported PDF settings rather than assuming MINT(31)=0.

The kinematics, common factor, alpha_s and event history are still supplied by
the reference. This is composed component validation, not native MPI event
generation, a statistical distribution test, or proof of final jet agreement.

To replay an identically instrumented trace directory:

```sh
python3 tools/analyze_mpi_channel_trace.py \
  build/jewel-legacy-mpi-channels-probe TRACE_DIRECTORY BASELINE_HEPMC result.json \
  --width-config reference/legacy-gluon-decays.dat \
  --pdf-probe build/jewel-legacy-mpi-pdf-probe
```
