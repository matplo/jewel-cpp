# Native Pythia8 input and hadronization boundary

This is an implemented adapter milestone, not a complete JEWEL generator and
not a passing Fortran/Pythia8 jet-physics comparison. JEWEL evolution is not
inserted into this round trip yet. The [machine-readable report](validation-pythia8.json)
records all settings, counts and library/data hashes.

Validated locally with Pythia8 **8.317**, C++17, AppleClang and macOS arm64.
The input executable links Pythia8 and system C++ libraries; it needs neither
Python nor Fortran at runtime. Its optional build uses one explicit Pythia
installation prefix for headers, library and XML data. This host's library was
built for macOS 26.6 while the compiler's deployment target is 26.0; the linker
reported that mismatch, and the binaries executed successfully on this host.
This does not establish portability to older macOS releases.

## Input adapter

`Pythia8Input` produces native `HardEventSnapshot` objects for pp hard QCD at
5020 GeV, 20 < pT-hat < 100 GeV, internal PDF choice 13 and tune 14. FSR, ISR,
MPI and hadronization are disabled explicitly. This is a reduced controlled
configuration; it is not JEWEL's default ISR/MPI physics or a matched Pythia6 tune.

Snapshots preserve the complete Pythia event record, source indices, stable
IDs within the event, mothers/daughters, colour, junction declarations, weight,
incoming fractions, scales and process code. Generator vertices retain **mm**
units and are not mislabeled as the JEWEL medium production point in fm.

Hard incoming/outgoing roles follow documented absolute statuses 21/23.
The original outgoing entries are not necessarily the final partons: all 2,000
hard legs in this sample have final beam-remnant-recoil copies. The adapter
follows unique reciprocal copy chains with statuses 62/64 to identify
`hard_final`; it does not rank partons by pT. Branching or unsupported history
is rejected rather than silently interpreted as an ISR-free event. The original
hard pair remains available for process identity and momentum-closure checks.

Native execution:

```sh
cmake -S . -B build-pythia8 -DCMAKE_BUILD_TYPE=Release \
  -DJEWEL_WITH_PYTHIA8=ON -DJEWEL_PYTHIA8_ROOT=/path/to/pythia8/8.317
cmake --build build-pythia8 -j4
build-pythia8/jewel-pythia8-input hard-input.jsonl 1000 2600317
```

The executable refuses to overwrite an existing output. JSONL has a run record,
one event/failure record per attempt, and a final summary. It is a development
input schema, not HepMC or an existing hepyy-utils analysis table.

## Separate hadronizer

`Pythia8Hadronizer` imports a complete colour-connected set, including needed
remnants, into its own Pythia instance and calls `forceHadronLevel(false)`.
It does not call hard generation. Native showers, resonance showers, process
resonance decays and colour reconnection are explicitly disabled. Supplied
junctions are retained rather than inferred. The present samples have no
junctions, so junction-bearing topologies still need dedicated validation.

The hadronizer uses tune 14 with hadron decays enabled and a 10 mm proper-lifetime
limit. This is an explicit Pythia8 validation policy, not a claim that Pythia6
fragmentation and decay choices have been reproduced. Resonance input needs a
separate policy and is currently rejected. Input must be finite and on shell;
JEWEL string construction and any required on-shell projection remain separate
work. No missing colour endpoint is invented to rescue a failed input.

The output retains the complete hadronization history and maps imported entries
back to their input stable IDs. Final particles must be colourless; total charge
and four-momentum must close, including all final particles in the comparison.
Failure returns remain explicit and count toward attempted hadronizations.

## Measured results

- 1,000 input attempts: 1,000 snapshots, zero generation failures, 2,000 hard legs.
- Complete input final-state four-momentum residual: at most 5.01e-10 GeV.
- 1,000 separate hadronizations: 1,000 returned events, zero failures, 89,976 final particles.
- Hadronization four-momentum residual: at most 6.10e-7 GeV at 5020 GeV collision energy.
- Repeated independent engines with the same seeds reproduce particles and history exactly on this build.
- Negative controls reject off-shell input and report an open colour line as a failed hadronization.

All 21 C++/Pythia8 contract tests pass in the optional build. The 1,000-event
boundary executable also runs each hadronization through a second independently
seeded-identically instance as a reproducibility check. Neither multiplicity
nor conservation checks prove JEWEL physics equivalence.

```sh
ctest --test-dir build-pythia8 --output-on-failure
build-pythia8/pythia8-boundary-test 1000 boundary.json
python3 tools/summarize_pythia8.py hard-input.jsonl boundary.json \
  /path/to/pythia8/8.317 docs/validation-pythia8.json
```

## Next integration gates

Initialize JEWEL virtualities, formation times and production geometry from
these final hard copies; connect the existing cascade without Pythia FSR;
construct colour-complete strings after medium evolution, including the selected
recoil/subtraction policy; and validate hadronization with common partonic
states against Pythia6/JEWEL. ISR/MPI interpretation, additional channels,
weighted biasing, LHAPDF/nuclear PDFs and custom JEWEL output still require
implementation. Only then can the requested full jet/substructure ratio gate
be evaluated for the standalone generator.
