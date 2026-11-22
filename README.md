# jewel-cpp

An experimental standalone C++17 port of JEWEL 2.6.0. Pythia8 supplies hard events, optional initial-state radiation (ISR), and hadronization. The C++ port implements the JEWEL cascade and expanding optical medium, with a native multiple-parton interaction (MPI) path under validation. This is an independent port with GPL-2.0-or-later licensing. See [NOTICE](NOTICE), [LICENSE](LICENSE), and [GUIDELINES](GUIDELINES).

## Build and test

Use CMake 3.18 or newer, a C++17 compiler, Python 3 and installations of Pythia8 and LHAPDF. The macOS ARM64 GitHub Actions workflow uses Pythia 8.317, LHAPDF 6.5.5 and the CT14nlo PDF set. Set the environment variables below to the installation prefixes and the directory containing your LHAPDF sets.

```sh
export LHAPDF_DATA_PATH=/path/to/share/LHAPDF
cmake -S . -B build-pythia8 -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON \
  -DJEWEL_WITH_PYTHIA8=ON \
  -DJEWEL_PYTHIA8_ROOT=/path/to/pythia8 \
  -DJEWEL_LHAPDF_ROOT=/path/to/lhapdf
cmake --build build-pythia8 --parallel 4
ctest --test-dir build-pythia8 --output-on-failure
```

The 54 native tests run on PRs into `main` and on pushes to `main`. The initial Linux x86-64 build exposed numerical differences in three frozen MPI replay comparisons. The CI workflow uses the Apple Silicon platform used for validation and retains the replay tolerances. The independent Fortran differential tests use the frozen JEWEL source described in the [porting notes](docs/porting-notes.md#build-and-validate).

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

## Inclusive jet R_AA

Activate a Conda environment with the [hepyy wheel bundle](https://github.com/matplo/hepyy/tree/main/demos), a working cppyy runtime and `hepyy-utils`. For example, start with `conda activate /Users/ploskon/hepbundle`. Check the runtime with `python -c 'import hepyy; hepyy.load("fastjet")'`. Use the wheel prefixes for the build above if compiling against this environment.

```bash
packages=$(python -c 'import sysconfig; print(sysconfig.get_path("purelib"))')
export LHAPDF_DATA_PATH="$packages/hepyy_lhapdf/share/LHAPDF${LHAPDF_DATA_PATH:+:$LHAPDF_DATA_PATH}"
# Run this if CT14nlo is missing from your LHAPDF data paths.
lhapdf install CT14nlo
# Use these as the Pythia8 and LHAPDF roots in the CMake command above.
printf '%s\n' "$packages/hepyy_pythia8" "$packages/hepyy_lhapdf"
```

On macOS, the native wheels were tested with Apple Clang. If Conda injects compiler flags or an alternate C++ runtime, use a fresh CMake build directory and configure with `env -u CXXFLAGS -u CPPFLAGS -u LDFLAGS cmake ... -DCMAKE_CXX_COMPILER="$(xcrun --find clang++)"`. The tested analysis environment used Python 3.14 and a source-built cppyy backend.

From the repository root, reuse the Bash `common` array from the quickstart. This runs independent pp and zero-interaction Pb–Pb samples for closure, plus a smaller quenched sample to demonstrate the output.

```bash
runner=build-pythia8/jewel-run-events
"$runner" raa-pp.jsonl 3500 2600611 vacuum "${common[@]}"
"$runner" raa-control.jsonl 3500 2600612 zero-interaction "${common[@]}" --centrality 0 10
"$runner" raa-medium.jsonl 300 2600514 medium "${common[@]}" --centrality 0 10
taa=(--taa 23.35 --taa-error 0.20 --taa-source 'ATLAS arXiv:1805.05635 Table 1 (0–10%)')

python tools/analyze_native_events.py raa-pp.jsonl raa-control.jsonl \
  --generator "$runner" --output raa-control-jets.json
python tools/jet_raa.py raa-control-jets.json "${taa[@]}" \
  --validate-no-quenching --output raa-closure.json

python tools/analyze_native_events.py raa-pp.jsonl raa-medium.jsonl \
  --generator "$runner" --output raa-medium-jets.json
python tools/jet_raa.py raa-medium-jets.json "${taa[@]}" --output raa.json
```

For each jet pT bin, the tool multiplies the weighted jet count by Pythia's hard-process cross section and divides by the total generated weight and bin width. Failed evolution attempts retain their generated weight and contribute zero jets. The Pb–Pb yield per inelastic event is the per-nucleon-pair spectrum times the class-averaged `T_AA`. The example uses `23.35 ± 0.20 mb⁻¹` from [ATLAS Table 1](https://arxiv.org/pdf/1805.05635). The overlap uncertainty scales the yield and cancels in the model ratio because the same factor appears in its denominator.

This is a particle-level binary-scaling example for one hard-scale window with pp hard input in both samples. It uses a constant class mean overlap and does not reweight the medium by the event's impact parameter. The closure check requires at least three bins with 50 jets or more in each sample and checks consistency with unity within three standard errors. Empty denominator bins have no ratio. See the [closure results and Glauber follow-up](docs/analysis-examples.md#inclusive-jet-nuclear-modification-factor).

## Planned analysis examples

| Example | Scope |
|---|---|
| Inclusive jet `R_AA` | The quickstart above implements binary scaling and closure. The plan extends it to broader hard-scale coverage and impact-parameter weighting. |
| Soft Drop `z_g` and `theta_g` | Make central Pb–Pb medium/vacuum distributions and a separate ATLAS-style `r_g`-differential suppression comparison. |
| Proton and nuclear PDFs | Switch installed proton sets or members through LHAPDF, then validate a bound-nucleon configuration with explicit beam and isospin policy. |
| Pythia8 steering | Compare the same hard process with standalone Pythia8 and document which controls affect the hard input, ISR, MPI, and hadronization. |

The [demonstration plan](docs/analysis-examples.md) records the analysis definitions, steering options, and implementation gates. The [porting history](docs/porting-notes.md), [port plan](docs/port-plan.md), and [routine inventory](docs/routines.csv) contain the numerical validation and implementation details.
