# JEWEL vacuum and standalone Pythia8 steering

The [README example](../README.md#compare-with-standalone-pythia8) uses pp HardQCD at 5.02 TeV, a 50–150 GeV hard window, CT14nlo/0, power-5 pThat bias and Pythia 8.317. Each hadron-level sample contains 3,500 accepted events. The same-seed hard-input replay matched all 3,500 event weights and pThat values and the generated hard cross section exactly in each MPI setting. The generated cross section was 3.4174 µb with ISR only and 3.3918 µb with ISR and Pythia8 MPI.

The hadron-level comparisons use independent seeds. Both settings use Monash fragmentation, the matched decay policy and colour reconnection disabled. JEWEL evolves the final-state shower and exports strings to a separate Pythia8 hadronizer. Standalone Pythia8 evolves its own final-state shower. It interleaves ISR, FSR and MPI when MPI is enabled. These differences can change jet observables even with identical hard-input steering.

## Jet spectrum

The table gives the JEWEL-to-Pythia8 ratio of inclusive jet cross sections in each jet pT bin. Jet clustering uses hepyy FastJet, anti-kt R=0.4 and |η| < 2. Each spectrum uses the generated hard cross section, the bias-corrected weights and the full generated weight denominator. Errors combine the event-level spectrum and generator cross-section uncertainties.

| Jet pT (GeV) | ISR only | ISR + Pythia8 MPI |
|---|---:|---:|
| 40–50 | 1.020 ± 0.064 | 0.981 ± 0.064 |
| 50–60 | 1.092 ± 0.072 | 0.867 ± 0.058 |
| 60–80 | 0.806 ± 0.048 | 0.915 ± 0.055 |
| 80–100 | 0.816 ± 0.069 | 0.952 ± 0.079 |
| 100–125 | 0.946 ± 0.081 | 0.896 ± 0.083 |
| 125–150 | 0.941 ± 0.092 | 0.849 ± 0.082 |
| 150–200 | 0.977 ± 0.176 | 1.042 ± 0.172 |

The relative deficit in 60–80 GeV jets is smaller with Pythia8 MPI enabled. At 50–60 GeV the ratio moves from above unity to below unity. These changes describe the two generator configurations and include how each treats the interplay of MPI and the final-state shower.

## Jet shapes at 60–100 GeV

| Input mode | Generator | Jets | Mean mass (GeV) | Mean pTD |
|---|---|---:|---:|---:|
| ISR only | JEWEL C++ | 2,570 | 10.344 ± 0.125 | 0.385 ± 0.004 |
| ISR only | Pythia8 | 2,581 | 10.633 ± 0.115 | 0.378 ± 0.003 |
| ISR + Pythia8 MPI | JEWEL C++ | 2,635 | 10.820 ± 0.112 | 0.383 ± 0.003 |
| ISR + Pythia8 MPI | Pythia8 | 2,515 | 11.333 ± 0.125 | 0.378 ± 0.004 |

The means use the bias-corrected weights with event-level statistical errors. Both comparisons give a lower mean mass for JEWEL. The pTD differences are comparable to the statistical errors.

[Machine-readable comparison with generator settings](pythia8-steering-comparison.json)
