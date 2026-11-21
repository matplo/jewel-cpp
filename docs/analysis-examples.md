# Analysis examples plan

Deliver each example as a runnable generator card or shell command, a Python analysis using hepyy/cppyy FastJet, and a small output with the cuts, weights, and generator settings recorded. Keep the 100-event smoke test separate from measurement-scale statistics.

## Inclusive jet nuclear modification factor

1. Use matched collision energy, particle selection, jet definition, and binning for the pp and Pb–Pb samples. Generate a broad hard-scale range or combine exclusive hard-scale slices, keeping the cross section and bias compensation for every slice.
2. Convert the pp spectrum to a cross section per jet pT bin. Establish the Pb–Pb yield per inelastic event in the selected centrality class, with the hard sample normalized consistently. Use the class-averaged nuclear overlap `<T_AA>` with consistent units in `R_AA = (dN_AA/dpT) / (<T_AA> dσ_pp/dpT)`.
3. Validate the normalization with quenching disabled and binary collision scaling enforced, then show recoil inclusion and the chosen background subtraction policy. Report statistical and overlap uncertainties separately. The existing interactive report measures event-weighted medium/vacuum jet yields and can serve as a regression baseline for the numerators and denominators.

## Soft Drop in central Pb–Pb

Use the [ATLAS substructure-dependent jet suppression measurement](https://doi.org/10.1103/PhysRevC.107.054909) at 5.02 TeV as the reference for `R_AA(pT, r_g)`. Start with anti-kt R=0.4, Cambridge/Aachen reclustering, Soft Drop beta=0 and zcut=0.2, then match the publication's particle and jet acceptance, centrality and bin definitions. Keep the ATLAS untagged `r_g=0` category separate.

Extend the analysis to expose the accepted splitting angle `r_g = ΔR` and `theta_g = r_g/R` alongside the existing `z_g`. Make grooming and selection configurable, and check tagged fractions and empty bins before forming ratios. Show both the change in yield and the tagged-jet shape ratio, identifying each normalization. For measured `z_g` and `theta_g` shape comparisons, use the observable definitions and fiducial cuts in the [ALICE measurement](https://arxiv.org/abs/2107.12984).

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
