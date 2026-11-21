# Full C++ JEWEL completion gates

The active objective is a complete standalone C++ JEWEL with Pythia8 where
needed, supporting every feature of jewel-custom-2.6.0. Component tests do not
satisfy the full objective. This document keeps the remaining acceptance scope
explicit while numerical routines are ported.

## Required implementation

- Complete medium model, all shower and scattering evolution, formation times,
  LPM and colour coherence, all ANGORD modes and first-splitting switches.
- Pythia8 hard-event generation/import, ISR and MPI handling, colour topology,
  hadronization, decays, and the supported collision channels (including EEJJ,
  vector bosons, photons and Drell–Yan where supported by the reference).
- All recoil/projection/subtraction modes and weighted event normalization.
- The custom recipe's production-point, parton and geometry exports and run
  functionality, audited against its actual patches rather than inferred from
  stock routine parity. Output compatible with hepyy-utils is preferred;
  HepMC3 is acceptable if the analysis can consume it correctly.
- Standalone execution without Python/Fortran in the production engine. Keep
  the frozen Fortran reference available only for validation.

## Event and analysis acceptance

Use frozen, recorded cards, PDFs, versions, settings, seeds and weights for
both generators. Compare equivalent MC settings. Separate changes caused by
Pythia6-to-8 input/fragmentation from changes in the JEWEL transport core, while
still evaluating the end-to-end target.

Each compared setting needs at least 1,000 selected jets from EACH generator.
Track generated and selected counts, weights, sum of weights squared, effective
sample size and cross-section normalization. More events may be necessary for
substructure and well-populated spectrum bins; 1,000 is a minimum, not a guarantee
of precision. Reuse native hepyy/cppyy jet finding in the analysis. Match in full
phase space before analysis cuts. Specify recoil subtraction explicitly.

Predeclare jet radii, acceptance, pT ranges, binning and substructure definitions.
Initial substructure targets are jet mass, girth, groomed mass and Soft Drop zg;
freeze grooming parameters and treatment of untagged jets before production.
For each accepted comparison, the C++/Fortran ratios must lie within 0.9–1.1
(or better). Report statistical uncertainties and all failing/undefined bins.
Do not remove failed bins or tune settings after inspecting the ratios to
manufacture agreement. Bins with insufficient reference statistics require
more data or a clearly incomplete validation status, not a pass.

## Report and audit

Produce a standalone dynamic HTML report with selectable generator settings,
medium/recoil/channel choices, observable and jet definition. Include overlaid
distributions, ratios with the 10% band, uncertainties, event/jet counts, run
provenance, explicit unsupported features and failed gates. Numerical-component
results should be separately browsable, never presented as event-level results.

Completion requires actual complete-event outputs, validated analysis artifacts,
passing feature coverage and the report. The currently committed numerical
parity reports establish only their explicitly listed component claims.
