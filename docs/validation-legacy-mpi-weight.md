# Native legacy MPI hard-overlap weight

`LegacyMpiTable` in `include/jewel/legacy_mpi.hpp` and `src/legacy_mpi.cpp`
ports the PYMULT mode-4 bin mapping and mode-5 hard-overlap survival weight.
The object owns the saved 20-bin sums, sample counts and VNT146 normalization.
Live screening, overlap enhancement and non-diffractive cross section are
supplied per evaluation. This separation preserves the measured reference
behavior when a later initializer changes screening without rebuilding the
saved table. No reference numerical constants are hard-coded in the component.

The returned diagnostics are bin coordinate, one-based bin, cross section above
the hard scale and survival probability. The implementation retains the source's
bin-coordinate clamps, cross-section denominator floor, exponential cap and
low-impact-parameter bypass when a Sudakov weight was already applied. Inputs
outside the supported finite, nonnegative domain are rejected. The optional
energy rescaling is an explicit multiplicative input; complete photon-beam
bookkeeping is not implemented or claimed here.

## Independent reference checks

The build extracts the original bin and weight expressions unchanged from the
hash-verified Pythia6 source distributed with JEWEL 2.6.0. The small Fortran
wrapper supplies arrays and scalar inputs; it does not use C++ intermediate
results. The reference source must be supplied externally.

The differential suite covers 1,804 configurations and 7,216 output values,
including adjacent doubles at all 19 internal bin edges, endpoint clamps,
zero overlap, the cross-section floor, the exponential cap, the Sudakov bypass,
nonuniform tables/counts and changed live screening with a fixed saved table.
All comparisons pass at relative tolerance 2e-12 and absolute tolerance 2e-13;
bin and zero/nonzero decisions must agree exactly. Maximum absolute differences
are 0 for coordinates/bins, 4.26326e-14 for the integrated cross section, and
6.66134e-16 for probability. Ten invalid-input cases are rejected.

The captured initial SIGM array is also exercised. Its per-bin count of 100 is
inferred from the documented initialization identity
`SIGSUM = sum(SIGM)/(20*MSTP(83))`; it is not fitted to the weight outputs.
The independent live trace below directly records NMUL and does not need that
inference.

## Replay of actual generator state

A read-only diagnostic records the first 1,000 mode-5 evaluations from the full
reference run, including actual SIGM, NMUL, VNT146, live screening, overlap,
non-diffractive cross section and original output. Feeding those inputs through
the native core reproduces all 4,000 scalar outputs exactly on this host.
The instrumented 1,000-attempt reference run reproduces its original HepMC
byte for byte, SHA-256
`64a5289388223a4fb5e1b272007a431c773b5125cb3277e66d6718733e275ca3`.
This establishes the component on real source state without changing its RNG.

See [differential results](validation-legacy-mpi-weight.json),
[live-state results](validation-legacy-mpi-weight-live.json), and the
[diagnostic patch](../reference/mpi_weight_trace.patch).

## Integration status

The component is compiled into `jewel_core` but is not selected by the event
runners. It does not create the legacy integration tables, sample the impact
parameter, generate successive MPI kinematics, or assign flavours and colour
connections. Passing this component does not establish full MPI or jet parity.

The [overlap normalization and hard-impact proposal](validation-legacy-mpi-overlap.md)
are now ported and independently validated. The next work is to port the
cross-section table initialization and remaining interaction sampling,
retaining the independently measured initialization and production scales.
Simply applying this weight on top of ordinary Pythia8 MPI would double-count
selection and is not a justified integration strategy. The full native/reference
jet and substructure gates remain failing.

Reproduce the component test:

```sh
cmake --build build --target jewel-legacy-mpi-probe jewel-legacy-mpi-reference
ctest --test-dir build --output-on-failure -R '^legacy-mpi-weight-differential$'
```

For the live replay, apply `reference/mpi_weight_trace.patch` to a copy of the
frozen Pythia6 source, rebuild the existing full reference replacing only its
Pythia6 object, and run unchanged cards into a fresh diagnostic directory.
Keep the source as `pythia.f`, executable as `reference`, patch as
`instrumentation.patch`, stdout as `run.log`, and the original card/output names.

```sh
python3 tools/analyze_mpi_weight_trace.py build/jewel-legacy-mpi-probe \
  /path/to/diagnostic-run /path/to/baseline/events.hepmc live-weight.json
```
