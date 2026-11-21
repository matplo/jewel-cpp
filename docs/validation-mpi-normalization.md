# Phase-aware MPI normalization and PDF-scale audit

This corrects the preceding MPI audit: its first 20 calls per MINT(35) were
initialization calls. MINT(35)=3 was incorrectly treated as production context.
A new diagnostic explicitly marks entry/exit of PYINIT and reproduces the
existing **1000-attempt Fortran HepMC byte for byte**.

PYINIT calls both PYMULT(1) and PYMIGN(1). In the first pp initialization, the
legacy initializer accepts pT0=2.58 GeV with an integral about 125 mb. The second
initializer rejects that scale (about 50.0 mb), then accepts pT0=2.33 GeV
(58.7021239168 mb). PARP(82) becomes 1.8 GeV: the shared effective screening is
**2.3261103202 GeV**, matching the preceding native run. VINT(145) contains the
second initializer's integral; it must not be labeled as the saved legacy one.
The [saved-state trace](validation-mpi-saved-state.md) now confirms that sampled
production entries retain the first legacy integral and all 20 sampling bins
while using the lowered shared screening scale.

All 20 explicitly marked production MPI calls use MINT(35)=1 and
**PDF Q² = pT²+pT0²**, as well as the shifted coupling scale. Standard Pythia8
uses unshifted PDF Q²=pT². The new trace has 80 evaluations, including those
20 production calls. All coupling values match the compiled Pythia8 translation
to maximum absolute difference 8.33e-17. No physics or random usage changed.

The original program initializes all four isospin combinations even when only
pp has requested events. The following values are from its first pp initialization.

| Cross section (mb) | Fortran pp | Pythia8 |
|---|---:|---:|
| Total | 86.0241104 | 86.0241104 |
| Elastic | 18.1345269 | 18.4382938 |
| Single diffraction, each side | 6.6889633 | 6.0647024 |
| Double diffraction | 8.7777668 | 7.7330157 |
| Nondiffractive | 45.7338901 | 47.7233961 |

Native nondiffractive normalization is about 4.35% larger. Both generators
require the MPI integral to exceed 1.1 times their nondiffractive cross section.
No cross section was retuned to force acceptance.

## Isolated shifted-PDF diagnostic

A temporary Pythia8 library changes only its compile-time `SHIFTFACSCALE`
constant from false to true. The installed library and standard native build
are unchanged. The source notes possible heavy-flavour threshold interpretation
issues: this remains a diagnostic, not a validated production option.

MPI pT0Ref=1.8 GeV, ecmRef=1800 GeV and power=.25 reproduce the measured effective
reference screening. Couplings use the verified translation. This diagnostic
accepts pT0=2.33 GeV without reduction, with a logged MPI integral of 149.42 mb.
It changes initialization and production PDF scales. Cuts, bins, weights and
attempted-event normalization remain unchanged.

It accepts **1498/1500 attempts**, yielding **2075 native jets versus 1357
Fortran jets**. Attempt 194 fails composed ISR transport; attempt 770 fails a
recoil-copy check. Both remain counted in normalization. All six observable
gates remain not passed.

| Jet pT (GeV) | Native / Fortran |
|---|---:|
| 10–15 | 1.131 |
| 15–20 | 0.927 |
| 20–30 | 0.986 |
| 30–40 | 0.975 |
| 40–60 | 1.290 |
| 60–80 | 0.889 |

Topology and energy-scaled momentum checks pass; 14 stricter component
excursions remain recorded, maximum 3.41e-7 GeV. This identifies a source
convention difference, not full MPI, heavy-flavour, shower or jet equivalence.
Overlap profiles, interleaving, source initialization state, residual ISR
mappings and other feature gaps remain unresolved.

## Evidence and reproduction

- [Interactive comparison](mpi-shifted-pdf-comparison.html)
- [Normalization and build provenance](validation-mpi-normalization.json)
- [Phase-tagged scales and couplings](validation-mpi-phases.json)
- [Topology and failures](validation-mpi-shifted-pdf-topology.json)
- [Phase marker patch](../reference/mpi_phase_core.patch) and
  [trace routines](../reference/mpi_phase_trace.f90), used with the preceding
  PYSIGH logging-call patch on temporary frozen-source copies.
- [Native normalization probe](../reference/mpi_normalization_probe.cpp), using
  the public SigmaTotal object after initialization.
- [Single-constant diagnostic patch](../reference/pythia8-mpi-shifted-pdf.patch).

Temporary artifacts are in `/private/tmp/jewel-mpi-phased-trace`,
`/private/tmp/jewel-mpi-normalization` and `/private/tmp/jewel-mpi-shifted-pdf`.
The shifted library recompiles MultipartonInteractions.cc using the original
`-O2 -std=c++11 -fPIC` flags and retains all other original Pythia8 objects.
The native runner is relinked to this isolated library, retaining the installed
library search path for the LHAPDF plugin. Source and library hashes are saved
because ordinary run settings do not encode this compile-time modification.
