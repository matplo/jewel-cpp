# Vacuum MPI and remnant comparison

The comparison uses 5.02 TeV pp dijets with a 20–100 GeV hard window, CT14nlo/0,
JEWEL vacuum evolution and final hadrons. The C++ runs use the mapped JEWEL
fragmentation profile and reference-derived coupling settings. The Fortran
JEWEL 2.6.0 run uses MPI and MPIFSR on. Common hepyy/FastJet analysis clusters
anti-kt R=0.4 before selecting pT > 10 GeV and |eta| < 2. Yields are divided by
original attempts, including the three Fortran discards. No shape
renormalization is applied.

| Sample | Attempts | Written | Selected jets | Yield / Fortran |
|---|---:|---:|---:|---:|
| Native MPI, common remnant recoil | 1250 | 1250 | 1732 | 1.021 |
| Pythia8 MPI, translated settings | 1250 | 1250 | 1741 | 1.027 |
| Pythia8 ISR only | 1250 | 1250 | 1651 | 0.973 |
| Native MPI, longitudinal remnant recoil | 1250 | 1250 | 1728 | 1.019 |
| C++ ISR and MPI off | 1250 | 1250 | 1502 | 0.885 |
| JEWEL 2.6.0 | 1000 | 997 | 1357 | 1.000 |

The native-MPI comparison passes the sample-size gate. None of the six
binned observables passes every 10% ratio check. Sparse or undefined bins
are included in that gate and should not be interpreted as well-resolved
physics discrepancies. The native-MPI ratios for pT 10–15 GeV, mass 6–8 GeV,
and pTD 0.6–0.7 are 1.165 ± 0.078, 0.766 ± 0.066, and 1.621 ± 0.319.
The pT ratios for 15–40 GeV are within 10%. Seven of eight zg bins are
within 10%, with the exception at 0.30–0.35 measuring 1.145 ± 0.143.
Uncertainties are one standard error from independent event samples.

## Attribution controls

The ISR-only control uses the same C++ path and Pythia8 MPI-off input.
Its 1,250 hard pT values match the native MPI run attempt by attempt.
Adding native MPI gives 81 more selected jets, 50 in the 10–15 GeV bin.
The paired changes per attempt are +0.0648 ± 0.0177 overall and
+0.0400 ± 0.0202 at 10–15 GeV. The low-pT ratio to Fortran rises from 1.052
to 1.165. The Pythia8 MPI alternative yields 1.183 in the same bin, so the
excess is shared by both MPI implementations in this comparison.

For the remnant sensitivity control, the default recoil scales all three
rest-frame momentum components. The alternate prescription preserves the
transverse components and scales the longitudinal component. Both then boost
the remnant system to the required total momentum. Hard pT and additional-pair
counts match on every attempt. The paired overall change is -0.0032 ± 0.0137
jets per attempt. The 6–8 GeV mass bin increases by 34 ± 19 jets, moving its
Fortran ratio from 0.766 to 0.871. The same deficit is present with Pythia8 MPI
(0.772) and with ISR only (0.788). The pTD excess at 0.6–0.7 also persists
across the four ISR-enabled C++ samples (1.50–1.68).

This isolates the effect of switching between two remnant recoil maps that
preserve Pythia8's remnant species. Both share that species model and the
Pythia8 hard process and ISR. The comparison cannot isolate the effect of
replacing Pythia8 remnants with PYREMN, or separate the shared hard, shower,
and fragmentation differences. The controls support an MPI contribution to
the low-pT excess and a possible recoil contribution to the mass deficit.

All C++ samples use seed 2600421. Use the command in
[the runner validation](validation-native-mpi-runner.md) with 1250 attempts.
For the two paired controls use `--isr-only` in place of `--native-mpi`, or add
`--mpi-longitudinal-recoil` to `--native-mpi`. The Pythia8 MPI comparison uses
`--experimental-isr-mpi --mpi-alpha-s .11754434294459773 --mpi-alpha-s-order 1
--mpi-screening 2 1800 .25 --mpi-double-gaussian .5 .4`.
The [interactive results](native-mpi-large-vacuum-comparison.html) and
[JSON results](native-mpi-large-vacuum-comparison.json) contain the common
binning, normalization and run metadata.
