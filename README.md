# jewel-cpp

An experimental standalone C++17 port of JEWEL 2.6.0. Pythia8 supplies hard events, optional initial-state radiation (ISR), and hadronization. The C++ port implements the JEWEL cascade and expanding optical medium, with a native multiple-parton interaction (MPI) path under validation. This is an independent port with GPL-2.0-or-later licensing. See [NOTICE](NOTICE), [LICENSE](LICENSE), and [GUIDELINES](GUIDELINES).

## Build and test

Use CMake 3.18 or newer, a C++17 compiler, Python 3 and installations of Pythia8 and LHAPDF. The GitHub Actions workflow uses Pythia 8.317, LHAPDF 6.5.5 and the CT14nlo PDF set. Set the environment variables below to the installation prefixes and the directory containing your LHAPDF sets.

```sh
export LHAPDF_DATA_PATH=/path/to/share/LHAPDF
cmake -S . -B build-pythia8 -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON \
  -DJEWEL_WITH_PYTHIA8=ON \
  -DJEWEL_PYTHIA8_ROOT=/path/to/pythia8 \
  -DJEWEL_LHAPDF_ROOT=/path/to/lhapdf
cmake --build build-pythia8 --parallel 4
ctest --test-dir build-pythia8 --output-on-failure
```

The 54 native tests run on PRs into `main` and on pushes to `main`. The independent Fortran differential tests use the frozen JEWEL source described in the [porting notes](docs/porting-notes.md#build-and-validate).

## Generate and analyze a small matched sample

This example runs the audited 5.02 TeV, 20–100 GeV hard window with Pb–Pb optical geometry in medium mode and pp hard input in both modes. It uses native MPI, power-5 hard-scale biasing, the mapped JEWEL fragmentation settings and CT14nlo. Use new output filenames for each run.

```bash
common=(--native-mpi --compact-hadrons --pthat-bias 5 --table-energy jewel
  --longitudinal-boost on --jewel-fragmentation --ecm 5020 --pt-min 20 --pt-max 100
  --lhapdf CT14nlo/0 --hard-alpha-s 0.11754434294459773 --hard-alpha-s-order 1
  --isr-alpha-s 0.11754434294459773 --isr-alpha-s-order 1 --alpha-s-nfmax 5 --isr-alpha-s-cmw off)
build-pythia8/jewel-run-events medium.jsonl 100 2600521 medium "${common[@]}" --centrality 0 10
build-pythia8/jewel-run-events vacuum.jsonl 100 2600521 vacuum "${common[@]}"
python tools/analyze_native_events.py medium.jsonl vacuum.jsonl --output jets.json
```

Jet analysis uses FastJet through hepyy/cppyy in an environment with `hepyy[cppyy,hepbundle]` and `hepyy-utils`. The small sample is a smoke test. See the [interactive Pb–Pb report](docs/central-pbpb-bias5.html) for statistically larger weighted medium/vacuum comparisons.

## Planned analysis examples

| Example | Scope |
|---|---|
| Inclusive jet `R_AA` | Build a matched pp cross-section and per-event Pb–Pb yield, with explicit Glauber normalization. |
| Soft Drop `z_g` and `theta_g` | Make central Pb–Pb medium/vacuum distributions and a separate ATLAS-style `r_g`-differential suppression comparison. |
| Proton and nuclear PDFs | Switch installed proton sets or members through LHAPDF, then validate a bound-nucleon configuration with explicit beam and isospin policy. |
| Pythia8 steering | Compare the same hard process with standalone Pythia8 and document which controls affect the hard input, ISR, MPI, and hadronization. |

The [demonstration plan](docs/analysis-examples.md) records the analysis definitions, steering options, and implementation gates. The [porting history](docs/porting-notes.md), [port plan](docs/port-plan.md), and [routine inventory](docs/routines.csv) contain the numerical validation and implementation details.
