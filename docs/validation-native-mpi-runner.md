# Native MPI in the event runner

The Pythia8/LHAPDF build of `jewel-run-events` supports an experimental
`--native-mpi` mode for pp at 5020 GeV and the 20–100 GeV hard-process window.
Pythia8 supplies the hard collision, ISR and outgoing beam remnants with its
MPI disabled. The C++ initializer runs both stages with a separate random
stream. Native MPI evolution uses the saved legacy overlap and the final
screening from the secondary stage. The resulting pairs enter the JEWEL
cascade as additional subsystems.

The remnant system supplies the energy and longitudinal momentum taken by the
new pairs. In its original rest frame a common scale changes the magnitudes
of the remnant three-momenta while preserving their directions and invariant
masses. A boost brings the system to the required total four-momentum.
The adapter verifies closure before it passes the updated state to the
cascade. It preserves the Pythia8 remnant species and uses their colour
connections to construct the outgoing string segments. This is an approximate
remnant recoil prescription. It does not reproduce PYREMN or jointly sample
the remnant composition and native MPI. The runner uses the audited proposal
bound and phase-space coefficients. Hard-process Sudakov reweighting and
independent bound optimization remain outside this integration.

The 100-attempt vacuum comparison uses CT14nlo/0, the mapped JEWEL fragmentation
profile, the JEWEL table energy option, and ISR on. The Fortran reference uses
MPI and MPIFSR on. Both samples wrote all 100 events. The native runner added
439 pairs in 84 events. Its legacy integral was 119.6644 mb with independent
random draws, and production screening was 2.32611 GeV.

The output topology audit passes colour and four-momentum closure. The largest
hadronization momentum component residual was 4.2e-7 GeV. Five events exceeded
the separate 1e-7 GeV absolute diagnostic for longitudinal momentum, while all
passed the energy-scaled criterion.

The common hepyy/FastJet analysis clusters anti-kt R=0.4 in full phase space
and selects pT > 10 GeV and |eta| < 2. It finds 153 native and 133 reference jets.
The integrated selected yield ratio is 1.15. None of the six binned observable
checks passes the 10% criterion. These samples are below the 1,000-jet
validation threshold. See [side-by-side distributions](native-mpi-runner-comparison.html)
and [machine-readable results](native-mpi-runner-comparison.json).

```sh
build-pythia8/jewel-run-events output.jsonl 100 2600421 vacuum \
  --native-mpi --lhapdf CT14nlo/0 \
  --hard-alpha-s .11754434294459773 --hard-alpha-s-order 1 \
  --isr-alpha-s .11754434294459773 --isr-alpha-s-order 1 \
  --alpha-s-nfmax 5 --isr-alpha-s-cmw off \
  --jewel-fragmentation --table-energy jewel --longitudinal-boost on
```
