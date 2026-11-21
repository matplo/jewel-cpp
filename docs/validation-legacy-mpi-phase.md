# Legacy MPI phase-space proposal

`sample_legacy_mpi_phase_space` in `src/legacy_mpi.cpp` now translates the
massless ISUB=96 hadron/hadron phase-space proposal used by PYMULT. The input
is the proposed xT squared and explicit mixture coefficients COEF(96,1),
COEF(96,8) and COEF(96,9). These coefficients are inputs, not guessed tune
constants. Outputs are tau, subsystem rapidity, scattering-angle cosine and
the two required beam momentum fractions.

Both tau branches and all three rapidity mappings preserve their source
boundary comparisons. The ISUB=96 rapidity limits are plus/minus half log(tau),
rather than the general hard-process cuts. Five random draws are consumed:
tau-mixture choice, tau value, rapidity-mixture choice, rapidity value and
scattering-angle sign. Momentum availability is a separate check using the
source's 0.01 remaining-fraction reserve, after all five draws.

## Validation

The reference extractor verifies the frozen Pythia6 hash, extracts the PYMULT
production statements unchanged, and supplies the corresponding ISUB=96
PYKLIM limits and hadron/hadron PYKMAP mapping. The wrapper fixes MINT(47)=4
and ISTSB=9. This is explicitly a hadronic, massless proposal check, not a
validation of general PYKLIM/PYKMAP or lepton/photon beam modes.

All 2,445 cases and 17,115 scalar outputs agree exactly on this host. The matrix
covers five coefficient configurations, xT squared from 1e-10 through one,
random proposals, all relevant mixture boundaries and adjacent doubles, and
the angle-sign boundary. The availability tests place remaining fractions
at the independently evaluated Fortran x+0.01 boundary, adjacent doubles and
offsets of 1e-10, spanning both tau branches and rapidity modes.

There are 1,496 available and 949 unavailable cases. Availability decisions and
five-draw consumption agree exactly; nine invalid inputs are rejected. The
numerical tolerance remains relative 2e-12 and absolute 2e-13, while discrete
decisions and zero/nonzero agreement are required independently.

All four MPI component differential suites pass together: weight, overlap,
scale and phase space. [Machine-readable results](validation-legacy-mpi-phase.json).

```sh
cmake --build build --target jewel-legacy-mpi-phase-probe jewel-legacy-mpi-phase-reference
ctest --test-dir build --output-on-failure -R '^legacy-mpi-(weight|overlap|scale|phase)-differential$'
```

## Remaining integration

The native components now cover overlap initialization/proposal, hard-event
weighting, decreasing interaction scales and massless phase-space proposals.
The differential PYSIGH-equivalent cross section, its sampling Jacobian,
adaptive mixture/bound initialization, acceptance, and flavour/colour event
construction remain required before this becomes an event-generation backend.
Pythia8 still supplies MPI in the current runner. No full-event or jet-parity
claim follows from these component checks.
