# Fragmentation mapping and full-generator configuration audit

Recorded 2026-11-21. This adds an opt-in mapping of JEWEL's explicit INITPYTHIA
fragmentation overrides into Pythia8. It does not claim Pythia6/Pythia8
fragmentation equivalence or close the full-generator comparison gate.

## Implemented mapping

The default remains Monash (`Tune:pp=14`). Select the mapped profile explicitly:

```sh
build-pythia8/jewel-run-events mapped.jsonl 20 2601001 medium --jewel-fragmentation
```

The C++ API is `HadronizerConfig::fragmentation =
FragmentationProfile::Jewel260Mapped`. The effective setting strings are recorded
in the run header. The profile starts from Monash and overrides the following
parameters; all other engine settings retain their prior values.

| JEWEL/Pythia6 parameter | Value in source | Pythia8 setting or transformation |
|---|---:|---|
| PARJ(1) | 0.073 | StringFlav:probQQtoQ |
| PARJ(2) | 0.2 | StringFlav:probStoUD |
| PARJ(3) | 0.94 | StringFlav:probSQtoQQ |
| PARJ(4) | 0.032 | StringFlav:probQQ1toQQ0 |
| PARJ(11) | 0.31 | StringFlav:mesonUDvector = p/(1−p) |
| PARJ(12) | 0.4 | StringFlav:mesonSvector = p/(1−p) |
| PARJ(13) | 0.54 | StringFlav:mesonCvector and mesonBvector = p/(1−p) |
| PARJ(25) | 0.63 | StringFlav:etaSup |
| PARJ(26) | 0.12 | StringFlav:etaPrimeSup |
| PARJ(21) | 0.325 | StringPT:sigma |
| PARJ(41) | 0.5 | StringZ:aLund |
| PARJ(42) | 0.6 | StringZ:bLund |
| PARJ(46), source default | 1 | StringZ:rFactC |
| PARJ(47), MSTJ(11)=5 | 0.67 | StringZ:rFactB |

The meson transformation is essential: the bundled Pythia6 code chooses the
multiplet using `INT(PARJ(11..13)+PYR(0))`, so PARJ is a probability. Pythia8
`FlavourSelection.xml` defines the corresponding parameters as the
vector/pseudoscalar **ratio**. Pythia6's PYZDIS uses PARJ(46) for charm and
PARJ(47) for bottom with MSTJ(11)=5; Pythia8's `ZSelection.xml` documents the
corresponding Bowler factors.

JEWEL assigns default-REAL literals into double-precision PARJ entries. The
mapping preserves those rounded source values before computing ratios. This
is parameter correspondence, not a promise of identical fragmentation models,
particle tables, popcorn treatment, string-corner behavior or decay branching
fractions. Other inherited defaults, including diquark fragmentation terms,
still need a complete audit. Numerical equality of the mapped controls is not
sufficient evidence for hadron-level agreement.

## Validation performed

The fragmentation contract checks inverse ratio-to-probability conversion,
charm/bottom factors and 20 closed light/strange/charm/bottom strings through
Pythia8, with the existing charge and momentum checks. Existing hadronizer
boundary and shower-output tests also pass.

The mapped profile produces 20/20 accepted vacuum events and 18/20 medium
events for seed 2601001. Both medium failures remain in cascade evolution.
For every accepted event, native particles, initialized hard legs, archived
ancestry, thermal references and RNG draw counts match the previous Monash
run exactly. This verifies that the separate hadronizer does not alter cascade
randomness or state. [Sample audit and effective settings](validation-mapped-fragmentation.json).

The large native report still uses Monash. It has not been silently regenerated
or relabelled as using this profile.

## Configuration differences before a full Fortran comparison

The inspected Fortran source is the pinned stock `jewel-2.6.0.f.stock` and its
bundled `pythia6425mod-lhapdf6.f`. The installed LHAPDF `pdfsets.index` identifies
13100 as **CT14nlo**. Pythia8 8.317's installed `PDFSelection.xml` identifies its
option 13 as **NNPDF2.3 QCD+QED LO**, not the same set.

| Component | Fortran setup | Current reduced native runner | Required follow-up |
|---|---|---|---|
| Beam PDF | LHAPDF ID 13100, CT14nlo | Internal Pythia8 option 13, NNPDF2.3 LO | Explicit common PDF/member and coupling policy; record grid/library hashes |
| ISR | INITPYTHIA forces MSTP(61)=1 | Disabled | Port ISR histories; a reduced ISR-off reference must be labelled an explicit benchmark variant |
| ISRSCAT | Controls whether ISR partons interact | No ISR histories | Setting ISRSCAT=F does not disable Pythia6 ISR |
| MPI | Off by default | Off | Reduced setting aligns; MPI functionality remains unimplemented |
| Geometry | Centrality sampled, default 0–10% | Fixed b=0 | Expose shared geometry settings; fixed b=0 is not the 0–10% ensemble |
| Medium boost | Default on | Off | Match explicitly |
| Shower cutoff/couplings | Defaults Q0=1.5, LambdaPS=LambdaQCD=0.4 | Q0=1, both Lambdas=0.2 | Match via explicit Fortran cards/native configuration |
| Angular ordering | Default mode 1 | Mode 0 | Match explicitly; cover all requested modes separately |
| Recoils | Default excluded, hard cut 5 GeV | Included, hard cut 0 | Match inclusion, cuts and subtraction policy |
| Fragmentation | Explicit JEWEL overrides plus Pythia6 defaults | Monash or new mapped overrides plus Pythia8 defaults | Equivalent-parton fragmentation study; audit remaining defaults |
| Coupling from PDF | Modified Pythia6 reads PDF metadata with PDFALPHAS fallback | Pythia8 tune's process coupling | Inspect actual initialization output; PDFALPHAS alone does not prove equality |

The existing installed Pythia8 prefix contains its LHAPDF6 plugin, and CT14nlo
is installed, so a common external PDF is technically available. A subsequent [external-PDF milestone](validation-external-pdf.md) implements
explicit selection and coupling controls, with CT14nlo integration checks.
The legacy hard-process coupling prescription still needs comparison. No full-generator sample with these mismatches is labelled
“same settings” in the report.
