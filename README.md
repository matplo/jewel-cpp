# jewel-cpp

An experimental standalone C++17 port of JEWEL 2.6.0. Pythia8 supplies hard events, optional initial-state radiation (ISR), and hadronization. The C++ port implements the JEWEL cascade and expanding optical medium, with a native multiple-parton interaction (MPI) path under validation. This is an independent port with GPL-2.0-or-later licensing. See [NOTICE](NOTICE), [LICENSE](LICENSE), and [GUIDELINES](GUIDELINES).

[Project documentation and validation reports](https://matplo.github.io/jewel-cpp/)

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

## Generate JEWEL events in Python

Activate the Conda environment or virtual environment you’ll use to run JEWEL.
With cppyy available in that environment, install JEWEL and the hepbundle wheels
from PyPI:

```bash
pip install 'hepyy[hepbundle,jewel-cpp]'
python -c 'import hepyy; hepyy.load("jewel_cpp"); import jewel_cpp'
```

If your environment needs cppyy from pip, use
`pip install 'hepyy[cppyy,hepbundle,jewel-cpp]'`.

From this checkout, run the [FastJet script](examples/jewel_fastjet.py) with
`python examples/jewel_fastjet.py --events 10`, or open the
[FastJet notebook](examples/jewel_fastjet.ipynb) with a kernel from the same
environment.

### hepyy recipe fallback

The recipe builds the library against the environment’s Pythia8 and LHAPDF
wheels and registers it as `jewel_cpp`. It requires Git, CMake, a C++17
compiler, and GitHub SSH access.

```bash
pip install 'hepyy[hepbundle]'
hepyy recipe update
hepyy install jewel-cpp
```

Use `hepyy[cppyy,hepbundle]` in the pip command if your source-build environment
needs cppyy from pip.

### Manual CMake fallback

Install the same `hepyy[hepbundle]` dependencies as for the recipe, then run
the examples in that environment.

Run the commands below from the root of the `jewel-cpp` checkout. On macOS, see the Apple Clang note under [Inclusive jet R_AA](#inclusive-jet-r_aa) if Conda supplies its own compiler flags.

```bash
packages=$(python -c 'import sysconfig; print(sysconfig.get_path("purelib"))')
cmake -S . -B build-cppyy -DCMAKE_BUILD_TYPE=Release \
  -DJEWEL_WITH_PYTHIA8=ON -DJEWEL_WITH_CPPYY=ON \
  -DJEWEL_PYTHIA8_ROOT="$packages/hepyy_pythia8" \
  -DJEWEL_LHAPDF_ROOT="$packages/hepyy_lhapdf"
cmake --build build-cppyy --parallel 4
cmake --install build-cppyy --prefix "$PWD/install-cppyy"
export HEPYY_SYSTEM_PACKAGES_DIR="$PWD/install-cppyy/share/jewel-cpp/hepyy${HEPYY_SYSTEM_PACKAGES_DIR:+:$HEPYY_SYSTEM_PACKAGES_DIR}"
python examples/jewel_fastjet.py --events 10
```

For a manual build, set `HEPYY_SYSTEM_PACKAGES_DIR` in each shell that runs the examples. Launch Jupyter from the configured environment so its kernel inherits the registry path.

Both examples use `hepyy.load(["jewel_cpp", "fastjet"])`. Each call to `jewel_cpp.EventGenerator.next()` attempts one event and returns its weight, hard scale, and final particles. The event owns its particle collection, so you can retain it across calls. A rejected hard event or failed cascade or hadronization returns `accepted=False` with a stage and error message. `hard_statistics()` reports the Pythia8 hard-input counters and cross section.

The in-process API uses the same JEWEL cascade and string preparation as the command-line runner. It supports vacuum, a fixed-impact-parameter expanding medium, and a zero-interaction control. Set `config.mode = "medium"` and `config.impact_parameter_fm` for the expanding medium. Configure the proton hard input, PDFs, ISR and Pythia8 MPI through `config.input`. Final particles use GeV. Use the command-line runner for Glauber event sampling, native MPI, geometry sidecars, and thermal subtraction.

### Jet-aligned medium geometry

The [medium geometry notebook](examples/jewel_medium_geometry.ipynb) generates a
0–10% Pb–Pb sample at 5.02 TeV and selects reconstructed anti-`k_t` jets near
100 GeV. It aligns the leading jet north and shows the optical energy density,
the hard production vertex, the estimated hot portions of both hard-parton
rays, and recoil production sites. It also plots weighted event averages and
the stored thermal-reference momenta. Build `jewel-run-events` and
`jewel-medium-probe` to run it. [Path-length definitions](docs/validation-geometry-lengths.md)

The Glauber-weighted example used 1,000 attempts in 0–10% Pb–Pb.
Of 999 accepted events, 452 had a leading jet at 75–125 GeV and 419 of
those leading jets matched a hard leg. See the
[cumulative plots](docs/figures/jewel-glauber-cumulative.png),
[single-event display](docs/figures/jewel-glauber-event.png), and
[machine-readable run summary](docs/jewel-glauber-1000.json).

### Compare parton and hadron jets on the same event

The [parton-to-hadron notebook](examples/jewel_parton_hadron_jets.ipynb)
clusters the exact Pythia8 string inputs and their final hadrons from one
full `jewel-run-events` record. It matches jets before applying cuts. The
medium example also clusters the hadrons after subtracting incoming
thermal-reference momenta and dropping residual thermal entries.

The published comparison uses 1,000 accepted events each in vacuum and
Glauber-weighted 0–10% Pb–Pb. For 644 stable Pb–Pb pairs, subtraction shifts
the matched jet pT by −2.21 ± 0.09 GeV on average before hadron-level cuts.
[Paired response and subtraction plots](https://matplo.github.io/jewel-cpp/#parton-hadron) ·
[Run summary](docs/parton-hadron-1000.json).

## Inclusive jet R_AA

Activate a Python environment and install the JEWEL, Glauber, and hepbundle
wheels. The analysis also uses `hepyy-utils` and needs cppyy for FastJet.

```bash
pip install 'hepyy[hepbundle,jewel-cpp,glauber-cpp]'
pip install hepyy-utils
# Run this if cppyy is missing from your environment.
pip install 'hepyy[cppyy]'
```

From the repository root, generate minimum-bias Pb–Pb geometry at 5.02 TeV
with the [ROOT-free Glauber CLI](https://matplo.github.io/glauber-cpp/#run).
The command writes `events.csv`, `centrality.csv`, and `metadata.json` into a
new directory. The default 5% bins align with the 0–10% selection below.

```bash
glauber-pbpb pbpb-5020 60000 72913
glauber=pbpb-5020/events.csv
```

Build the `jewel-run-events` command from the checkout against the Pythia8
and LHAPDF wheels. The JEWEL wheel provides the in-process Python API, and
this example uses the command-line runner for Glauber sampling and native MPI.
Make the CT14nlo grid visible through `LHAPDF_DATA_PATH` before generating events.

```bash
packages=$(python -c 'import sysconfig; print(sysconfig.get_path("purelib"))')
cmake -S . -B build-raa -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF \
  -DJEWEL_WITH_PYTHIA8=ON \
  -DJEWEL_PYTHIA8_ROOT="$packages/hepyy_pythia8" \
  -DJEWEL_LHAPDF_ROOT="$packages/hepyy_lhapdf"
cmake --build build-raa --target jewel-run-events --parallel 4

# If CT14nlo is missing, download the grid into a local PDF data directory.
mkdir -p pdfsets
curl -fL https://lhapdfsets.web.cern.ch/current/CT14nlo.tar.gz -o pdfsets/CT14nlo.tar.gz
tar -xzf pdfsets/CT14nlo.tar.gz -C pdfsets
export LHAPDF_DATA_PATH="$PWD/pdfsets:$packages/hepyy_lhapdf/share/LHAPDF${LHAPDF_DATA_PATH:+:$LHAPDF_DATA_PATH}"
```

On macOS, if Conda injects compiler flags or an alternate C++ runtime, use a
fresh build directory, prefix the configure command with
`env -u CXXFLAGS -u CPPFLAGS -u LDFLAGS`, and add
`-DCMAKE_CXX_COMPILER="$(xcrun --find clang++)"`.

On macOS, use a working Conda cppyy runtime if you already have one. With
pip cppyy on a Mac using the macOS 26 SDK, select a compatible SDK for the
Python analysis. The Python 3.15 verification used the macOS 15.4 SDK. When
the pip wheel also reports a missing `libzstd.1.dylib`, make Conda's copy
visible to the loader. For example, with the Conda environment active before
activating the venv:

```bash
export SDKROOT=/Library/Developer/CommandLineTools/SDKs/MacOSX15.4.sdk
cppyy_lib_dir=$(mktemp -d)
ln -s "$CONDA_PREFIX/lib/libzstd.1.dylib" "$cppyy_lib_dir/libzstd.1.dylib"
export DYLD_LIBRARY_PATH="$cppyy_lib_dir${DYLD_LIBRARY_PATH:+:$DYLD_LIBRARY_PATH}"
```

Reuse the Bash `common` array from [the matched sample](#generate-and-analyze-a-small-matched-sample).
Run the pp and zero-interaction samples for the closure test, then generate
the quenched sample for the medium prediction.

```bash
runner=build-raa/jewel-run-events
"$runner" raa-pp.jsonl 3500 2600611 vacuum "${common[@]}"
"$runner" raa-control.jsonl 3500 2600612 zero-interaction "${common[@]}" --centrality 0 10 --glauber-events "$glauber"

python tools/analyze_native_events.py raa-pp.jsonl raa-control.jsonl \
  --generator "$runner" --output raa-control-jets.json
python tools/jet_raa.py raa-control-jets.json \
  --validate-no-quenching --output raa-closure.json

"$runner" raa-medium.jsonl 300 2600514 medium "${common[@]}" --centrality 0 10 --glauber-events "$glauber"
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

## Steer proton and nuclear PDFs with LHAPDF

Use a Pythia8 build with LHAPDF6 enabled. Activate the hepyy wheel environment used in the examples above and make the desired grids visible through `LHAPDF_DATA_PATH`. Each PDF flag takes `SET/MEMBER`. Member 0 is the central fit for the sets below. Member 1 selects the first error member.

```bash
for set in CT14nlo CT18ANLO EPPS21nlo_CT18Anlo_Pb208; do
  lhapdf install "$set"
done
input=build-pythia8/jewel-pythia8-input
hard=(--ecm 5020 --pt-min 50 --pt-max 150 --isr on
  --hard-alpha-s 0.118 --hard-alpha-s-order 2
  --isr-alpha-s 0.118 --isr-alpha-s-order 2)
"$input" ct18-central.jsonl 1000 2601101 "${hard[@]}" --lhapdf CT18ANLO/0
"$input" ct18-member1.jsonl 1000 2601101 "${hard[@]}" --lhapdf CT18ANLO/1
"$input" ct14-central.jsonl 1000 2601101 "${hard[@]}" --lhapdf CT14nlo/0

lead=EPPS21nlo_CT18Anlo_Pb208/0
"$input" lead-hard.jsonl 1000 2601101 "${hard[@]}" --lhapdf CT18ANLO/0 \
  --lhapdf-hard-a "$lead" --lhapdf-hard-b "$lead"
```

| Option | Pythia8 setting and scope |
|---|---|
| `--lhapdf SET/MEMBER` | Sets `PDF:pSet` for the common proton baseline, including ISR and the hard process by default. |
| `--lhapdf-hard-a SET/MEMBER` | Enables `PDF:useHard` and sets `PDF:pHardSet` for beam A. |
| `--lhapdf-hard-b SET/MEMBER` | Enables `PDF:useHard` and sets `PDF:pHardSetB` for beam B. |

A beam without a hard override uses the common baseline even when the other beam has an override. Either hard flag can take a proton or nuclear set. For an EPPS21 nuclear error member, change `$lead` to `EPPS21nlo_CT18Anlo_Pb208/1` on both beams. Those grids already average protons and neutrons per nucleon in lead, so there is no extra isospin weighting. Beam IDs remain proton IDs in the event record. The nuclear override applies to the hard process. ISR uses the baseline PDF and the native MPI path has its own CT14nlo setup. [EPPS21 grid conventions](https://research.hip.fi/qcdtheory/nuclear-pdfs/epps21/) · [Pythia8 PDF settings](https://pythia.org/latest-manual/PDFSelection.html)

External PDF choices require explicit `--hard-alpha-s` and `--hard-alpha-s-order`. Keep the hard and ISR couplings fixed when isolating the PDF change. HardQCD uses leading-order matrix elements in this example.

The same hard PDF flags work in the hadron-level runner. Reuse `runner`, `glauber`, and `lead` from above to combine the lead hard PDFs with the 0–10% Pb–Pb medium.

```bash
"$runner" lead-medium.jsonl 100 2601102 medium --compact-hadrons --isr-only \
  --ecm 5020 --pt-min 50 --pt-max 150 --table-energy jewel \
  --lhapdf CT18ANLO/0 --lhapdf-hard-a "$lead" --lhapdf-hard-b "$lead" \
  --hard-alpha-s 0.118 --hard-alpha-s-order 2 --isr-alpha-s 0.118 --isr-alpha-s-order 2 \
  --centrality 0 10 --glauber-events "$glauber"
```

To check the free-proton limit, run the validation script with the same hepyy environment. It replaces both hard PDFs with the same free-proton baseline and checks that the same-seed event records and cross section are identical. It also checks the recorded beam densities against direct LHAPDF queries for sets, members, and one- or two-sided overrides.

```bash
python tools/validate_lhapdf_steering.py "$input" --output lhapdf-validation.json
```

With Pythia 8.317 and LHAPDF 6.5.5, all 1,000 free-proton events matched exactly. Across eight configurations, all 16,000 hard PDF values agreed with LHAPDF, with a maximum absolute difference of `7.11e-15`. [Validation output](docs/lhapdf-steering-validation.json)

## Compare with standalone Pythia8

Run pp HardQCD at 5.02 TeV with a 50–150 GeV hard-scattering window, CT14nlo/0, Monash fragmentation and power-5 pThat bias. Activate the hepyy wheel environment above. Use the same Pythia8 build for the C++ runner and the standalone Python example.

```bash
runner=build-pythia8/jewel-run-events
steering=(--compact-hadrons --ecm 5020 --pt-min 50 --pt-max 150
  --pthat-bias 5 --table-energy jewel --lhapdf CT14nlo/0
  --hard-alpha-s 0.118 --hard-alpha-s-order 2
  --isr-alpha-s 0.118 --isr-alpha-s-order 2 --isr-alpha-s-cmw off)
"$runner" jewel-isr.jsonl 3500 2601201 vacuum "${steering[@]}" --isr-only
"$runner" jewel-mpi.jsonl 3500 2601202 vacuum "${steering[@]}" --experimental-isr-mpi
python tools/pythia8_steering_example.py generate jewel-isr.jsonl pythia-isr.jsonl --seed 2601211
python tools/pythia8_steering_example.py generate jewel-mpi.jsonl pythia-mpi.jsonl --seed 2601212
python tools/analyze_native_events.py jewel-isr.jsonl pythia-isr.jsonl jewel-mpi.jsonl pythia-mpi.jsonl \
  --output steering-jets.json
python tools/pythia8_steering_example.py compare steering-jets.json steering-comparison.json
```

The standalone script first repeats the JEWEL hard input with the same seed and checks the acceptance, event weights, pThat and hard cross section. It then enables Pythia8 final-state radiation and hadronization and uses an independent seed for the jet comparison. The comparison uses the same FastJet anti-kt R=0.4 clustering, |η| < 2 and neutrino rejection. It reports the cross-section-normalized jet spectrum and weighted jet mass and pTD means. [Measured comparisons](docs/pythia8-steering-comparison.md)

| Stage | jewel-cpp | Standalone Pythia8 |
|---|---|---|
| Hard process | HardQCD with proton beams and tune 14. Steer `--ecm`, `--pt-min`, `--pt-max`, `--pthat-bias`, the positional seed, the [LHAPDF options](#steer-proton-and-nuclear-pdfs-with-lhapdf) and `--hard-alpha-s` / `--hard-alpha-s-order`. | `HardQCD:all`, `Beams:*`, `PhaseSpace:pTHatMin/Max`, `PhaseSpace:bias2Selection*`, `Random:seed`, `PDF:*` and `SigmaProcess:alphaS*`. |
| ISR | `--isr-only`, `--experimental-isr-mpi` and `--native-mpi` enable Pythia8 ISR. Set `--isr-alpha-s`, `--isr-alpha-s-order`, `--isr-alpha-s-cmw` and `--alpha-s-nfmax`. | `PartonLevel:ISR`, `SpaceShower:alphaS*` and `StandardModel:alphaSnfmax`. |
| MPI | `--experimental-isr-mpi` uses Pythia8 MPI, with `--mpi-alpha-s`, `--mpi-alpha-s-order`, `--mpi-screening` and `--mpi-double-gaussian`. `--native-mpi` uses JEWEL's separate initializer and evolution, currently restricted to the audited 5.02 TeV, 20–100 GeV hard window. | `PartonLevel:MPI` and `MultipartonInteractions:*` control Pythia8 MPI. |
| Final-state shower | JEWEL evolves the imported partons in `vacuum`, `zero-interaction` or `medium` mode. `--angular-ordering` steers the JEWEL cascade. | `PartonLevel:FSR` and `TimeShower:*` steer the Pythia8 cascade. With ISR and MPI enabled, Pythia8 interleaves all three. |
| Hadronization | A separate Pythia8 instance fragments the JEWEL strings. Select Monash or `--jewel-fragmentation`. Use `--pi0-decay`, `--decay-tau0-max` and the supported `--hadrotype` modes. The ISR/MPI import path uses `--hadrotype 0`. | `HadronLevel:all`, the tune, `StringFlav:*`, `StringPT:*`, `StringZ:*`, `ParticleDecays:*` and particle decay switches. The example copies the JEWEL fragmentation and decay settings and disables colour reconnection in both runs. |

The runner exposes the settings above as command-line flags. The standalone script passes the recorded settings to Pythia8 through `readString`. Both configurations are recorded in the comparison JSON. [Pythia 8.3 manual](https://arxiv.org/abs/2203.11601)

## Planned analysis examples

| Example | Scope |
|---|---|
| Inclusive jet `R_AA` | The quickstart above implements binary scaling, Glauber-weighted impact parameters and closure. The plan extends it to broader hard-scale coverage. |

The [demonstration plan](docs/analysis-examples.md) records the analysis definitions, steering options, and implementation gates. The [porting history](docs/porting-notes.md), [port plan](docs/port-plan.md), and [routine inventory](docs/routines.csv) contain the numerical validation and implementation details.
