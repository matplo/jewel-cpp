# First larger Fortran/native jet comparison

This is a zero-interaction expanding-background comparison, not a stock vacuum
backend validation. The native `vacuum` run label currently means scattering
scale zero while retaining the expanding background, vertex sampling and
lifetime. Stock `medium-vac.f` instead returns zero lifetime/temperature/density
and a fixed maximum scatterer. To compare like controls, the reference here
uses `medium-simple.f` with XSECSCALEFAC=0. This distinction remains an open
runner design issue.

[Interactive side-by-side distributions and ratios](zero-interaction-comparison.html)
include jet pT, mass, girth, pTD and Soft Drop mass/zg, per-event uncertainties,
the 0.9–1.1 band, CSV export, settings and provenance. The numerical 10% gate
**does not pass**, and full physics matching remains unproven.

| Sample | Attempts | Written/hadronized | Selected jets |
|---|---:|---:|---:|
| Native C++ / Pythia8 | 1,500 | 1,467 | 2,137 |
| JEWEL 2.6.0 / Pythia6 | 1,000 | 999 | 1,359 |

Both use hepyy/cppyy FastJet, full-phase-space anti-kt R=0.4 followed by pT>=10
GeV and |eta|<2. Neutrinos are excluded consistently. Every failed attempt
contributes zero jets; there is no yield or shape rescaling. These counts exceed
the requested minimum, but do not establish sufficiently precise tail bins.

Representative native/Fortran jet-pT yield ratios are 1.158 (10–15 GeV),
0.898 (15–20), 1.046 (20–30), 1.252 (30–40), and 0.896 (40–60). The full
report retains sparse/undefined bins and statistical errors. Neither undefined
bins nor uncertainty bands turn an out-of-range central ratio into a pass.

## Aligned controls and remaining distinctions

The runs use pp at 5020 GeV, pThat 20–100 GeV, CT14nlo member 0, unweighted
generation, ISR/MPI enabled and the external Pythia final shower disabled.
JEWEL settings include NF=3, Q0=1 GeV, both Lambda values 0.2 GeV, ANGORD=0,
AOFIRSTSPLIT=false, coherence enabled, HADROTYPE=0, retained recoils without
rescattering or subtraction, and zero recoil cuts. The expanding medium uses
A=208, tau_i=.4 fm, T_i=.59 GeV, T_c=.17 GeV, 0–10% centrality, rapidity limit
2 and longitudinal boost enabled. Native tables use the source EOVEST formula.
The [reference cards](zero-interaction-reference.params) and
[medium cards](zero-interaction-medium.params) are retained.

The native run selects the mapped JEWEL fragmentation parameters and requests
hard alpha_s(MZ)=.118 at order 2. Mapping parameters does not prove equivalence
of Pythia6/8 fragmentation or coupling implementations. ISR/MPI models and
defaults, decay tables, channel initialization, record ordering and junction
handling remain distinct. The native generator rejects unsupported inputs.
Initialization geometry is independently sampled, not numerically identical.
These limitations are recorded in the machine-readable configuration audit
inside [the comparison](zero-interaction-comparison.json).

## Reference diagnostic repair

The first custom Fortran run terminated while reporting a string-construction
failure near attempt 908. Its PRINTPARTONS FORMAT line is missing separators.
`tools/build_jewel_comparison_reference.py` verifies frozen source hashes and
builds an isolated stock-physics reference. It removes the already-known
duplicate WEIGHT declaration and inserts the two missing FORMAT commas. No
physics statements are changed. [The build manifest](zero-interaction-reference-build.json)
records exact replacements, commands, source and binary hashes. Installed
recipes and binaries were not modified.

The repaired reference completes with one discarded event (attempt 908),
zero HepMC conversion failures and a complete end marker. Its final pp cross
section is 0.13720106285199746 mb; the native estimate is
0.13718533301421046 +/- 0.001777383904515948 mb. Cross-section agreement alone
does not establish shower or jet agreement.

## Complete attempt normalization

`tools/import_reference_hepmc.py --run-log jewel.log` can now restore explicitly
logged discarded attempt indices. This is restricted to unweighted PP events
with unit HepMC weights, matching NEVENT/NJOB, complete unique in-range discard
indices, a consistent total discard count and no HepMC conversion losses.
It emits a failed record at each logged index and associates the written events
with the remaining ordered attempts. It does not infer undocumented failures
or overwrite a previously imported file. Without a log, the previous written-event
policy remains in place and count mismatches are rejected.

Tests verify unchanged particle content, the restored attempt association,
unit-weight sums and rejection of incomplete, duplicate, out-of-range,
conversion-loss, weighted and mixed-isospin ledgers. The existing import
corruption/overwrite checks and all seven comparison-gate tests pass.

Reproduction uses the commands and paths in the JSON provenance. Build with
`tools/build_jewel_comparison_reference.py`, run the supplied cards in a new
directory, then import with `--run-log`, analyze both inputs together using
`tools/analyze_native_events.py`, compare with `tools/compare_jet_samples.py
--pair 0:1`, and render with `tools/render_jet_comparison.py`.

## Subsequent runner naming

The [vacuum backend update](validation-vacuum-backend.md) gives the `vacuum`
argument its actual JEWEL meaning. Use `zero-interaction` for the historical
setting in this report. The stored samples keep their original run labels and
have not been relabelled as genuine vacuum-backend evidence.
