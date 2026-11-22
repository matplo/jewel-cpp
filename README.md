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

From the repository root, reuse the Bash `common` array from the quickstart. Obtain `events.csv` from a complete minimum-bias run of the standalone [ROOT-free glauber-cpp generator](https://github.com/matplo/glauber-cpp), or use its bundled validation sample. The example uses its Pb–Pb sample at 5.02 TeV, validated against TGlauberMC. Set `glauber` to that sample or to a new run with the same profile and energy. The requested centrality boundaries must align with the CSV bins.

```bash
runner=build-pythia8/jewel-run-events
glauber=../glauber-cpp/validation/native-run/events.csv
"$runner" raa-pp.jsonl 3500 2600611 vacuum "${common[@]}"
"$runner" raa-control.jsonl 3500 2600612 zero-interaction "${common[@]}" --centrality 0 10 --glauber-events "$glauber"
"$runner" raa-medium.jsonl 300 2600514 medium "${common[@]}" --centrality 0 10 --glauber-events "$glauber"

python tools/analyze_native_events.py raa-pp.jsonl raa-control.jsonl \
  --generator "$runner" --output raa-control-jets.json
python tools/jet_raa.py raa-control-jets.json \
  --validate-no-quenching --output raa-closure.json

python tools/analyze_native_events.py raa-pp.jsonl raa-medium.jsonl \
  --generator "$runner" --output raa-medium-jets.json
python tools/jet_raa.py raa-medium-jets.json --output raa.json
```

The runner samples Glauber rows within the requested class with probability proportional to `N_coll`. This gives the Pb–Pb hard events their binary-collision-weighted impact-parameter distribution. JEWEL still samples the hard production point and evolves the jet using its optical medium at that impact parameter. The analysis uses the original Pythia event weights, since the impact-parameter weight has already been applied by sampling.

For each jet pT bin, multiply the weighted jet count by the Pythia hard-process cross section and divide by total generated weight and bin width. A failed evolution contributes zero jets and retains its generated weight in the denominator. For Pb–Pb, multiply the resulting per-nucleon-pair spectrum by the **unweighted** minimum-bias class mean `T_AA` recorded in the run header. Its fixed-bin statistical error scales the Pb–Pb yield and cancels in the model ratio. If using the optical-only runner, supply `--taa` and `--taa-source` explicitly to retain the class-mean-only calculation.

The example uses pp hard input and one hard-scale window. The closure gate uses at least three bins with 50 jets or more per sample and requires consistency with unity within three standard errors. Empty denominator bins have no ratio. See the [Glauber and R_AA validation report](docs/glauber-raa-validation.html).

## Soft Drop in 0–10% Pb–Pb at 5.02 TeV

Activate the same hepyy environment and reuse `common`, `runner`, `glauber`, and `raa-pp.jsonl` from the inclusive example. Use a fresh filename for the larger quenched sample.

```bash
"$runner" soft-drop-medium.jsonl 3500 2600514 medium "${common[@]}" \
  --centrality 0 10 --glauber-events "$glauber"
python tools/analyze_native_events.py raa-pp.jsonl soft-drop-medium.jsonl \
  --generator "$runner" --soft-drop-zcut 0.2 --output soft-drop-jets.json
python tools/jet_soft_drop.py soft-drop-jets.json --pt-edges 40 60 80 100 \
  --output soft-drop.json
```

The analyzer uses hepyy FastJet to cluster anti-kt R=0.4 jets, then declusters their Cambridge/Aachen trees with Soft Drop β=0. It records the accepted momentum split `zg`, the rapidity–azimuth angle `rg`, and `theta_g = rg/R`. Each shape ratio divides the Pb–Pb tagged-jet density by the pp tagged-jet density in the same jet pT interval, with `rg < 0.4` in both samples. Tagging fractions are reported separately.

The same output includes ATLAS-style `R_AA(pT, rg)` from cross sections and Glauber-normalized yields, with a separate untagged category. The result uses the model's 20–100 GeV hard window and |η| < 2. ATLAS measures jet pT > 158 GeV and |y| < 2.1, so the comparison is qualitative. The [Soft Drop report](docs/glauber-soft-drop.html) shows the quenched ratios alongside an independent zero-interaction control. [ATLAS measurement](https://arxiv.org/abs/2211.11470)

### Direct ATLAS comparison with thermal subtraction

The [ATLAS comparison report](docs/atlas-soft-drop-comparison.html) shows 158–200, 200–315, and 315–501 GeV jets from HEPData Table 18, with finer jet-pT bins from Table 14. The two hard-scattering windows are 100–700 GeV for the lower bins and 150–1000 GeV for the highest bin. Both use `--isr-only` because the native MPI calibration is limited to 20–100 GeV. The lifetime setting of 8.994 mm corresponds to 30 ps. The jet analyzer applies |y| < 2.1 and excludes muons and neutrinos.

Keep recoil hadronization enabled. The runner subtracts the stored incoming thermal momenta of the scatterers after hadronization. The analysis drops leftover thermal entries before jet clustering and Soft Drop. The raw comparison reuses the same events and weights.

```bash
atlas=(--isr-only --compact-hadrons --pthat-bias 5 --table-energy jewel
  --longitudinal-boost on --jewel-fragmentation --ecm 5020
  --lhapdf CT14nlo/0 --hard-alpha-s 0.11754434294459773 --hard-alpha-s-order 1
  --isr-alpha-s 0.11754434294459773 --isr-alpha-s-order 1 --alpha-s-nfmax 5
  --isr-alpha-s-cmw off --decay-tau0-max 8.99377374 --subtraction-radius 0.4)
analysis=(--generator "$runner" --atlas-particle-level --soft-drop-zcut 0.2)
atlas_sample() {
  local name=$1 lower=$2 upper=$3 seed=$4
  local hard=(--pt-min "$lower" --pt-max "$upper")
  local geometry=(--centrality 0 10 --glauber-events "$glauber")
  "$runner" "$name-pp.jsonl" 8000 "$seed" vacuum "${atlas[@]}" "${hard[@]}" || return
  "$runner" "$name-medium.jsonl" 8000 "$((seed+1))" medium "${atlas[@]}" "${hard[@]}" "${geometry[@]}" || return
  "$runner" "$name-control.jsonl" 8000 "$((seed+2))" zero-interaction "${atlas[@]}" "${hard[@]}" "${geometry[@]}" || return
  python tools/analyze_native_events.py "$name-pp.jsonl" "$name-medium.jsonl" "${analysis[@]}" \
    --exclude-thermal-residuals --output "$name-medium-jets.json" || return
  python tools/analyze_native_events.py "$name-pp.jsonl" "$name-control.jsonl" "${analysis[@]}" \
    --exclude-thermal-residuals --output "$name-control-jets.json" || return
  python tools/analyze_native_events.py "$name-pp.jsonl" "$name-medium.jsonl" "${analysis[@]}" \
    --raw-hadrons --output "$name-raw-jets.json"
}
atlas_sample atlas 100 700 2600921
atlas_sample atlas-high 150 1000 2601021

curl -fsSL 'https://www.hepdata.net/download/table/ins2512925/Table%2014/json' -o atlas-table-14.json
curl -fsSL 'https://www.hepdata.net/download/table/ins2512925/Table%2018/json' -o atlas-table-18.json
# The HTML renderer uses matplotlib.
python tools/atlas_soft_drop_report.py atlas-medium-jets.json atlas-control-jets.json atlas-raw-jets.json \
  atlas-table-14.json atlas-table-18.json atlas-soft-drop-comparison.html \
  --high-samples atlas-high-medium-jets.json atlas-high-control-jets.json atlas-high-raw-jets.json
```

## Planned analysis examples

| Example | Scope |
|---|---|
| Inclusive jet `R_AA` | The quickstart above implements binary scaling, Glauber-weighted impact parameters and closure. The plan extends it to broader hard-scale coverage. |
| Proton and nuclear PDFs | Switch installed proton sets or members through LHAPDF, then validate a bound-nucleon configuration with explicit beam and isospin policy. |
| Pythia8 steering | Compare the same hard process with standalone Pythia8 and document which controls affect the hard input, ISR, MPI, and hadronization. |

The [demonstration plan](docs/analysis-examples.md) records the analysis definitions, steering options, and implementation gates. The [porting history](docs/porting-notes.md), [port plan](docs/port-plan.md), and [routine inventory](docs/routines.csv) contain the numerical validation and implementation details.
