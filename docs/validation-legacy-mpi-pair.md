# Legacy MPI accepted-pair generation

`sample_legacy_mpi_pair` in `src/legacy_mpi.cpp` ports the PYMULT mode-6
accept/reject decision and subsequent two-parton flavour and momentum construction.
It takes the effective proposed cross section, overestimate, collision energy,
scaled transverse momentum squared, incoming fractions, scattering angle,
PARP(85), PARP(86), PARJ(2), PARU(2), and an owned random stream.
The cross section supplied by the caller must already include any applicable
source diffractive factor. That factor is not calculated by this routine.

The source does NOT choose the outgoing pair by sampling the individual ISUB=96
channel weights. It uses the summed cross section to reject proposals, then
uses a separate phenomenological flavour prescription. Replacing that step with
weighted channel selection would change the reference physics.

Draw order is preserved:

1. Reject if cross section < bound times the acceptance draw. Equality accepts;
   overestimate violations also accept, as in the source (no probability clamp).
2. Draw RFLAV, calculate transverse momentum, then draw azimuth.
3. If RFLAV >= max(PARP(85), PARP(86)), draw light flavour with
   `1 + int((2 + PARJ(2)) * u)` and assign the antiparticle to the second leg.
   Otherwise construct two gluons.

Both massless four-momenta use the original source operation order. The result
retains RFLAV for subsequent colour attachment. No colour tags, string-piece
selection, beam-remnant changes or showers are performed here. The API accepts
physical finite inputs; it restricts probability parameters to [0,1] and the
strange weight to [0,1]. These are explicit supported-domain constraints.

## Validation

The [reference extractor](../reference/extract_mpi_pair.py) verifies the full
bundled Pythia6 source SHA-256 and copies the acceptance, draw and pair-construction
statements unchanged into a small Fortran probe. Its wrapper supplies parameters
and a deterministic random tape. The C++ probe independently calls the native API.

[Results](validation-legacy-mpi-pair.json): 4,375 cases and 65,625 scalar comparisons
pass, with maximum absolute error 1.4210854715202004e-14 and unchanged tolerances
(relative 2e-12, absolute 2e-13). Acceptance, flavour IDs and consumed-draw counts
must match exactly. The matrix includes 2,000 deterministic randomized physical
configurations at collision energies 200, 2760, 5020 and 13000 GeV, and explicit
acceptance/flavour thresholds with adjacent representable doubles.
Subnormal text inputs are excluded because the probe stream parser rejects them.

The cases contain 1,468 rejections (one draw), 1,711 accepted gluon pairs
(three draws) and 1,196 accepted quark pairs (four draws). Independent checks
require positive energies, massless daughters and closure to the incoming
longitudinal four-momentum; maximum closure error divided by collision energy
is 2.842170943040401e-16. Thirteen invalid-input cases are rejected.
The [Pythia8 build](validation-legacy-mpi-pair-pythia8-build.json) passes the same
comparison. All nine selected native MPI component suites pass after relinking.

Reproduce:

```sh
cmake -S . -B build
cmake --build build --target jewel-legacy-mpi-pair-probe jewel-legacy-mpi-pair-reference
ctest --test-dir build --output-on-failure -R '^legacy-mpi-pair-differential$'
```

This adds a production C++ component, but it is not yet connected to a complete
native MPI event loop. Colour attachment, remnants, optional MPI showers, and
full-generator jet/substructure parity remain unfinished. No claim of complete
JEWEL equivalence follows from this component comparison.
