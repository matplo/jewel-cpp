# Pythia6 and Pythia8 on identical parton events

The saved vacuum native MPI events supply the same outgoing parton
momenta, masses, species, vertices and directed colour connections to both
hadronizers. The Pythia8 outputs come from the run that wrote these input
records. The paired driver converts the tags to reciprocal PYJETS pointers
and calls PYEXEC from the Pythia6 source bundled with JEWEL 2.6.0. It uses
JEWEL's INITPYTHIA fragmentation overrides and decay policy. The Pythia8 run
uses the mapped JEWEL settings. Engine-specific fragmentation and decay
implementations and random streams differ.

The production sample contains 12,500 attempts. One failed in C++ before
writing a hadronized event, leaving 12,499 common input events with no
junctions. The successful Pythia8 final records agree particle by particle with the first
12,500 attempts of the compact million-jet run. Both hadronizers pass momentum
closure on all 12,499 shared inputs.
The largest four-momentum component residual is 1.54e-6 GeV for Pythia6 and
7.42e-6 GeV for Pythia8. All three stages apply the same anti-kt R=0.4
selection and neutrino exclusion. Jets are selected independently at each
stage. Uncertainties on yield differences use paired events.

| Quantity | Before hadronization | Pythia6 | Pythia8 |
|---|---:|---:|---:|
| Selected jets | 20477 | 17526 | 17460 |
| Mean selected jet mass (GeV) | 1.729 | 4.811 | 4.717 |
| Mean selected pTD | .820 | .438 | .438 |
| Jets in mass 6–8 GeV | 272 | 2749 | 2692 |
| Jets in mass 8–12 GeV | 82 | 1206 | 1060 |
| Jets in pTD .6–.7 | 3052 | 765 | 797 |

Pythia8 minus Pythia6 is -57 ± 63 jets at mass 6–8 GeV and -146 ± 39 at
8–12 GeV. The 8–12 GeV bin shows a resolved 12.1% reduction on identical
input events. The 6–8 GeV change is about 2% and is statistically consistent
with zero. The pTD .6–.7 bin changes by +32 ± 38 jets, consistent with zero.
Both hadronizers make much larger changes relative to the parton-level sample.
This comparison measures fragmentation plus decays on the native parton
ensemble. It does not measure how the difference changes with a different
parton ensemble.

The [paired results](validation-paired-hadronization.json) include counts and
paired uncertainties for every mass and pTD bin. The driver is
`reference/paired_hadronization.f90`. Link it to the frozen Pythia6 object and
its LHAPDF libraries, then use `tools/paired_hadronization.py` to create the three
analysis inputs from a full runner export. Analyze them together with
`tools/analyze_native_events.py` and pass the resulting jets.json and audit.json
directory to `tools/summarize_paired_hadronization.py`.

## Findings from the two comparisons

The settings mapping preserves JEWEL's explicit Lund and flavour overrides.
Pythia6 expresses the vector-meson settings as probabilities, while Pythia8
takes vector-to-pseudoscalar ratios. The mapping also carries over the charm,
bottom and diquark Lund terms. The remaining engine defaults and the details
of string fragmentation and decays can still differ.

The million-jet vacuum comparison found C++/Fortran yield shifts of −7.8% at
mass 6–8 GeV, −10.5% at mass 8–12 GeV and +14.2% at pTD 0.6–0.7. In the paired
control the respective Pythia8/Pythia6 shifts were −2.1 ± 2.3, −12.1 ± 3.2,
and +4.2 ± 4.9 percentage points. The mass shift at 8–12 GeV is comparable to
the full generator difference. The other two paired shifts are statistically
consistent with zero. The shared native parton ensemble isolates the combined
effect of hadronization and decays for this ensemble.

[HTML summary](paired-hadronization-findings.html) ·
[Million-jet comparison](jewel-million-comparison.html)
