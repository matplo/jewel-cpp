# Surviving-parton selection and preparation

`prepare_surviving_partons` in `include/jewel/hard_initialization.hpp` ports
the `GENEVENT` status-selection block, stock lines 1267–1291. It operates on
surviving positive-status records in their source slot order. The caller
supplies the hard-pair indices, nominal masses and pseudorapidity limit.

The combined operation follows the source order:

1. Set each hard leg to status 1 when its current status is below 11.
2. Mark non-quark/gluon records below status 11 inactive (status 4).
3. For non-hard records below status 11, activate those with recorded mass
   greater than nominal mass times the promoted default-REAL `1.00001`.
4. Otherwise, activate quarks/gluons strictly inside the pseudorapidity limit,
   first calling `make_parton_massless` when their recorded mass is positive.
   Records outside the limit become status 4.

The above-nominal-mass branch precedes the pseudorapidity cut: it is not a
universal acceptance cut on all evolving partons. The strict comparison and
source single-precision rounding of `1.00001` are retained. Statuses 11 and
above remain unchanged. Literal `ISQORG` recognizes `abs(PDG)<6` or PDG=21,
including PDG zero and excluding top. An adapter must pass actual surviving
records, not unused PDG-zero padding. Each massless conversion updates the
same hard pair before the next record is processed.

The helper is transactional across the whole input range. If any conversion
fails, no earlier status or momentum changes are published. It consumes no
random numbers. It does not perform ancestry mapping, nominal-mass lookup,
`SETCOLOUR`, `FINDVIRTUALITIES` or MPI initialization. Those responsibilities
remain with the complete event adapter. The existing reduced native runner
has not been switched to this preparation path.

## Validation against the source block

The reference extractor checks the frozen source hash and wraps the unedited
`GENEVENT` block in a test-only subroutine with explicit COMMON initialization.
It also extracts unedited `ISQORG`, `MAKEMASSLESS`, `PYP` and `PYROBO` routines.
The reference nominal-mass lookup is a supplied PDG/antiparticle mass table;
this verifies selection arithmetic, not Pythia6/Pythia8 particle-data equality.

All **1,017 cases / 30,750 scalar values agree exactly** on the tested platform:

| Group | Cases | Scalar values |
|---|---:|---:|
| Selection boundaries | 990 | 29,700 |
| Ordered multiple conversions | 12 | 600 |
| Hard-line status setup | 15 | 450 |

The matrix spans light and heavy quarks, antiquarks, top, gluons, photons,
hadrons and the literal PDG-zero case; statuses 1, 2, 4, 10, 11 and 21;
zero mass, the exact nominal threshold and adjacent doubles; and coordinates
at/around eta=2 and outside acceptance. Multiple-conversion cases permute
three emitted records and change the hard-pair positions within the array.
All output statuses must match exactly; other values use the existing absolute
2e-13 / relative 2e-12 tolerance. Momenta, masses and auxiliary vertices are
compared in addition to statuses.

Contracts independently verify that an above-threshold out-of-acceptance
parton remains active, a near-on-shell out-of-acceptance parton is inactive,
a photon is inactive, and a late conversion failure restores the entire
record including earlier successful conversions.

All seven selected suites pass: preparation differential, massless contracts
and differential, kinematics differential, hard-initialization contracts and
differential, and MPI-initialization differential. [Evidence](validation-preparation.json)
records source/executable/input hashes and group results.

```sh
cmake --build build -j 4
ctest --test-dir build --output-on-failure \
  -R '^(preparation|massless|kinematics|hard-initialization|mpi-initialization)-(contracts|differential)$'
```

This validates the preparation block in isolation. The complete Pythia8 ISR/MPI
adapter, downstream evolution and matched jet/substructure comparison remain
outstanding; no full-event equivalence is claimed here.
