# jewel-cpp

An experimental standalone C++17 port of JEWEL 2.6.0, starting with a frozen-source numerical reference harness. This is not an official JEWEL release or a working event generator. GPL-2.0-or-later; see NOTICE, LICENSE and GUIDELINES.

## Build and validate

The core and probe executable need only CMake and a C++17 compiler. Python and gfortran are needed only for reference validation, not the C++ executable.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DJEWEL_REFERENCE_SOURCE=/path/to/jewel-2.6.0-source
cmake --build build -j 4
ctest --test-dir build --output-on-failure
cat build/validation.json
```

The reference directory must contain `jewel-2.6.0.f.stock` (preferred) or `jewel-2.6.0.f` with the exact hash in `reference/source.json`. Configuration rejects patched or unknown source. Only the selected unchanged routine bodies are extracted into the build directory; the reference executable does not need Pythia6 or LHAPDF. Do not enable fast-math or change Fortran's default REAL size. Omit `JEWEL_REFERENCE_SOURCE` for a C++-only build; that build runs contract tests but makes no reference-parity claim.

```sh
printf 'PQQ 3 0.5 0 0\nALPHAS 5 100 0.2 0\n' | build/jewel-probe
```

Rows contain `operation nf a b c`; output is `value random_draw_count`. For GETT, a/b/c are minimum transfer, maximum transfer, Debye mass; follow the row with a tape length on its own line and tape values on the next line. Shower and scattering Lambda default to 0.2 GeV in the probe. The library exposes separate values.

## Implemented milestone 1a

- Owned event storage with stable particle IDs, parent references, production coordinates, momentum and colour fields. It is an initial append-only graph; colour closure and evolution accounting are not implemented.
- Explicit RNG interface and finite replay tape, checkpoint/restore, exhausted-tape errors. No claim that a standard C++ RNG reproduces PYR; callers supply a recorded tape.
- Medium interface and zero-density vacuum implementation. The expanding optical medium and thermal-scatterer sampling remain to be ported.
- Direct translations of PQQ, PGQ, PGG, PQG, ALPHAS, INTPQQ, INTPGGLOW, INTPGGHIGH and GETT. GETT rejection sampling uses the same draw order and empty-interval convention.
- ALPHAS deliberately retains the Fortran single-precision beta-function coefficient before double-precision evaluation. Changing that is a separate numerical change.

The differential suite covers flavours 0/3/4/5/6, logarithmic scales, near-endpoint splitting fractions, near-Landau-pole values on the supported side, transfer intervals, forced rejection, and deterministic random tapes. Every implemented physics kernel is compared against the stock routine with absolute tolerance 2e-13 and relative tolerance 2e-12. GETT draw counts must match exactly. C++ domain-contract tests reject singular endpoints, nonfinite inputs, invalid scales and exhausted tapes; these are deliberate exceptions instead of relying on Fortran NaNs or hangs. No parity claim is made outside the documented physical domain.

## Reference harness and next gates

`reference/freeze.py` records and copies explicitly supplied source, executables, cards, patches and PDF files into a new baseline directory with SHA256 hashes. Label each baseline `stock`, `custom` or `perf`; use separate directories. Supply all relevant PDF members and generator inputs, not just a card that refers to mutable files. This tool freezes inputs; it does not certify sample completeness.

```sh
python3 reference/freeze.py --variant stock --output validation-output/stock-001 \
  --file /path/to/jewel-binary --file /path/to/params.dat \
  --file /path/to/medium.params.dat --file /path/to/pdf-member.dat
```

The full stages 0 and 1 remain open. Next implement optical-medium temperature/density and tables with independent probes, Ei interpolation and scattering/splitting integrals. Then add controlled initial-state snapshots and evolution replay. Before full-generator comparisons, capture stock and custom run manifests, complete PDF provenance, seeds, event weights, cross sections and rejected-event accounting; build a statistically adequate weighted baseline. Kernel parity is not validation of a full event generator.

Pythia8 hard-event import and separate hadronization adapters follow core evolution validation. No Pythia8, TGlauberMC or JETSCAPE dependency is introduced here. The full routine inventory and ordered port plan are preserved in `docs/port-plan.md` and `docs/routines.csv`.
