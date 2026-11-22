# Analysis examples plan

Deliver each example as a runnable generator card or shell command, a Python analysis using hepyy/cppyy FastJet, and a small output with the cuts, weights, and generator settings recorded. Keep the 100-event smoke test separate from measurement-scale statistics.

## Inclusive jet nuclear modification factor

1. Use matched collision energy, particle selection, jet definition, and binning for the pp and Pb–Pb samples. Generate a broad hard-scale range or combine exclusive hard-scale slices, keeping the cross section and bias compensation for every slice.
2. Convert the pp spectrum to a cross section per jet pT bin. Establish the Pb–Pb yield per inelastic event in the selected centrality class, with the hard sample normalized consistently. Use the class-averaged nuclear overlap `<T_AA>` with consistent units in `R_AA = (dN_AA/dpT) / (<T_AA> dσ_pp/dpT)`.
3. Validate the normalization with quenching disabled and binary collision scaling enforced, then show recoil inclusion and the chosen background subtraction policy. Report statistical and overlap uncertainties separately. The existing interactive report measures event-weighted medium/vacuum jet yields and can serve as a regression baseline for the numerators and denominators.

### First closure run

The [README example](../README.md#inclusive-jet-r_aa) uses one hard-scale window and a mean overlap for the centrality class. In the hepyy wheel environment, independent 3,500-attempt pp and zero-interaction Pb–Pb samples selected 7,388 and 7,575 jets, respectively. Seven bins met the 50-jet threshold. The largest deviation was 1.35 standard errors. At 60–80 GeV the control gave `R_AA = 1.037 ± 0.110`. The 300-attempt quenched sample exercised the output format.

### Glauber-weighted closure

The standalone ROOT-free Glauber CSV supplies minimum-bias Pb–Pb rows. The runner selects the centrality class and samples its impact parameters in proportion to event N_coll. The same class supplies the unweighted mean T_AA. This places the binary-collision weight in the geometry sampling once, while retaining Pythia's bias weights for the hard event. JEWEL still uses its optical medium and vertex sampler at the selected impact parameter.

For 0–10% Pb–Pb the 3,659 minimum-bias rows give T_AA = 23.1472 ± 0.0573 inverse mb, with the fixed-bin Monte Carlo error. The 3,500 zero-interaction events had mean b = 3.138 ± 0.020 fm, compared with 3.140 fm expected from N_coll weighting and 3.308 fm before weighting. The pp and zero-interaction runs selected 7,388 and 7,612 jets. All seven eligible bins passed the closure gate. The largest deviation was 1.19 standard errors, and the 60–80 GeV bin gave R_AA = 1.042 ± 0.111. The [HTML validation report](glauber-raa-validation.html) adds the 3,500-attempt quenched run, which gave R_AA = 0.772 ± 0.084 in the 60–80 GeV bin.

### TGlauberMC integration

TGlauberMC runs through ROOT. Start with a versioned hepyy recipe that pins the upstream source, depends on the existing ROOT recipe, and exposes a seeded Pb–Pb run through PyROOT. Validate minimum-bias centrality percentiles and `<T_AA> = <N_coll> / sigma_NN_inelastic` against published tables. Record the nuclear density, cross section and centrality policy alongside the result. Use that output to supply the mean overlap and, later, to validate impact-parameter weights against the chosen JEWEL geometry. [TGlauberMC reference](https://arxiv.org/abs/1710.07098)

Python access through the recipe is the first useful milestone. Wheels add a separate ROOT ABI and packaging task. Confirm the upstream code redistribution terms, choose a compatible ROOT distribution, and test library and dictionary loading for each supported platform before publishing wheels. The README uses the validated ROOT-free CSV as the geometry interface. Packaging the Glauber generator and exposing its API to Python can proceed independently of the JEWEL integration.

## Soft Drop in central Pb–Pb

The [runnable example](../README.md#soft-drop-in-010-pbpb-at-502-tev) uses anti-kt R=0.4 jets, Cambridge/Aachen reclustering, and Soft Drop β=0 with zcut=0.2. The shape ratios divide separately normalized tagged-jet densities in each jet pT interval. They use tagged jets with r_g < 0.4 and record the overall tagging fraction and r_g overflow. The r_g-dependent R_AA retains the absolute yield and cross section, with untagged jets in a separate category.

The [Soft Drop report](glauber-soft-drop.html) compares quenched and zero-interaction samples to the same pp baseline. The [ATLAS measurement](https://arxiv.org/abs/2211.11470) provides the reference for r_g-differential R_AA. Its jet momentum and acceptance differ from the example. The [ALICE measurement](https://arxiv.org/abs/2107.12984) provides measured Soft Drop shapes under its own charged-jet selection.

## LHAPDF inputs and nuclear beams

For an installed proton set, pass `--lhapdf SET/MEMBER` with explicit `--hard-alpha-s` and `--hard-alpha-s-order`. For a clean PDF-only study, hold the explicit coupling and other settings fixed and use `--isr-only` in the hadron runner. Compare `CT14nlo/0` with another installed member or a second proton set. Inspect the recorded `PDF:pSet` and coupling settings and track the event weights and cross section.

The current hard input uses proton beam IDs on both sides. The optical Pb–Pb geometry controls medium evolution. The native MPI setup uses its own internal CT14nlo choice. A nuclear PDF demonstration therefore needs explicit per-beam bound-proton PDF selection, proton/neutron composition and isospin handling, plus a coordinated MPI PDF policy when native MPI is enabled. Validate the free-proton limit, the beam flavour mixtures and the hard cross sections before claiming a Pb–Pb nuclear-PDF prediction.

## Steering compared with standalone Pythia8

| Component | Current controls and ownership |
|---|---|
| Hard input | Pythia8 HardQCD and Monash tune 14. Use `--ecm`, `--pt-min`, `--pt-max`, `--pthat-bias`, the seed argument, and the LHAPDF and hard coupling flags. |
| Initial-state radiation | Use `--isr-only`, `--experimental-isr-mpi`, or `--native-mpi` to enable the relevant input mode. The ISR coupling flags include `--isr-alpha-s`, `--isr-alpha-s-order`, `--alpha-s-nfmax`, and `--isr-alpha-s-cmw`. |
| Multiple interactions | The experimental Pythia8 MPI path exposes `--mpi-alpha-s`, `--mpi-alpha-s-order`, `--mpi-screening` and `--mpi-double-gaussian`. Native MPI uses a separate initializer audited at 5020 GeV and a 20–100 GeV hard window. |
| Final-state shower and medium | The JEWEL C++ cascade owns final-state evolution. Select `medium`, `vacuum`, or `zero-interaction` and use the JEWEL controls for centrality, angular ordering, recoils, and subtraction. |
| Hadronization | Pythia8 hadronizes the supplied string configuration. Select the default Monash profile or `--jewel-fragmentation`. Use `--pi0-decay on\|off` and the supported `--hadrotype` modes. |

The CLI exposes a selected set of controls. General Pythia8 cards, arbitrary `readString` settings, and beam identity changes require extending the adapter. For the demonstration, record both configurations, match the hard cuts and particle selection, and compare the vacuum and zero-interaction cases before enabling quenching.
