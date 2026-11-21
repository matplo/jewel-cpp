# Native MPI differential cross-section provider

`LegacyMpiCrossSection` in `include/jewel/legacy_mpi_cross_section.hpp` and
`adapters/legacy_mpi_cross_section.cpp` computes the massless, non-diffractive,
screened ISUB=96 cross section from proposal variables and explicit settings.
It composes native kinematics, scale selection, common factor, LHAPDF evaluation,
gluon widths and channel sums. Running alpha_s is an explicit callable; the
validated executable uses Pythia8 AlphaStrong with alpha_s(MZ)=0.11754434294459773,
order 1, nfmax 5 and CMW disabled.

The provider implements `LegacyMpiEvolution`'s callback signature directly:

```cpp
// provider, evolution and random are independently configured objects.
auto outcome = evolution.step(random, std::ref(provider));
```

The two objects must share collision energy and screening conventions. The
caller supplies the audited phase-space coefficients, channel flavour settings,
decay configuration, PDF scale floor and screening scale shift. The provider
calculates all downstream weights; it does not accept captured PDFs, common
factors, channel weights, alpha_s evaluations or annihilation-width sums.

## Initialization versus production kinematics

The first complete comparison failed for some initialization calls because
PYSIGH used an independent VINT(71) cut rather than the pT implied by the proposal.
The native configuration now has an optional `kinematic_pt_cut` for that case.
Production leaves it unset and derives pT = sqrt(s)/2 * sqrt(xT squared).
The comparison separately asserts that every captured production cut equals
this expression. Initialization supplies the captured cut as a kinematic input.
A future native initializer must reproduce its own cut-setting policy.

The calculation preserves PYSIGH's lower bound on s-hat, protected t/u
expressions, maximum of the cut and angular pT squared, and the screened scale
shift. The dimensionless sampling screen and scale shift are explicit separate
parameters, preserving the previously audited saved/live-state distinction.
The provider reproduces the hadron-beam endpoint return before PDF evaluation.

## Live reference comparison

[Evidence](validation-legacy-mpi-cross-section.json) covers 100 initialization
and 100 production calls from the frozen 1,000-attempt reference sample. The
read-only patch is `reference/mpi_native_cross_section_trace.patch`, used with
the existing phase markers. Instrumented HepMC remains byte-identical to the
baseline (SHA-256 64a5289388223a4fb5e1b272007a431c773b5125cb3277e66d6718733e275ca3).

All 81,160 scalar comparisons pass, and all channel counts/identities match:

| Quantity | Values | Maximum relative difference |
|---|---:|---:|
| Kinematics and PDF scale | 800 | 1.86e-16 |
| Pythia8 alpha_s | 200 | 3.50e-16 |
| Common factor | 200 | 0 |
| Gluon width | 200 | 0 |
| PDFs | 4,400 | 3.01e-16 |
| Channel totals | 200 | 9.90e-16 |
| Raw channel weights | 37,580 | 9.64e-16 |
| PDF-weighted channels | 37,580 | 1.20e-15 |

Relative and absolute tolerances remain 2e-12 and 2e-13, with finite values and
zero/nonzero agreement checked separately. Source, executable, cards, trace,
patch, input and decay-configuration hashes are recorded. All eleven selected
MPI regression suites also pass after building the provider in both configurations.
The live combined comparison uses the Pythia8-enabled build.

Run against the identically instrumented trace directory:

```sh
python3 tools/analyze_mpi_cross_section_trace.py \
  build-pythia8/jewel-legacy-mpi-cross-section-probe \
  TRACE_DIRECTORY BASELINE_HEPMC result.json \
  --width-config reference/legacy-gluon-decays.dat \
  --alpha-s .11754434294459773
```

The audited settings are CT14nlo/0, MSTP32=8, MSTP33=0, MSTP52=2, MSTP57=1,
and legacy evolution stages below 2. Nuclear PDFs, diffractive factors, modern
MPI companion reshaping and other scale-selection policies are not implemented
by this provider. Native proposals and settings replace captured ones in the
next composed sequence comparison; this test still uses reference proposals.
Complete initialization, hard-event colour/remnant integration, optional MPI
showers and final-state jet/substructure agreement remain unfinished.
