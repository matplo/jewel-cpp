# Native survivor preparation against JEWEL 2.6.0

`prepare_imported_partons` connects the imported Pythia8 final record to the
validated GENEVENT survivor-selection block. Call it after ISR timing attachment
and before hard/MPI virtuality initialization. It processes final particles in
native physical-slot order, using the primary hard pair for ordered
MAKEMASSLESS recoil. Anchors are excluded from the survivor range.

Nominal masses come from `Pythia8Input::nominal_mass`. The whole state commits
only after successful preparation and validation. A mass-provider failure test
checks that prior records are not partially activated or modified. Import
metadata, stable IDs, parent links, colours and attached production/decay times
survive preparation. The native runner's ISR/MPI guard remains in place.

## Generated-record comparison

Two 1,000-attempt PP samples use Pythia8 8.317, sqrt(s)=5020 GeV, pThat 20–100
GeV, ISR and MPI enabled, FSR/hadronization disabled, eta_max=2. Final records
use Pythia8 source order. ISR timing uses a fixed 0.5 replay tape. The probe
writes pre-preparation records as independent Fortran input and actual prepared
native output. The audit replays those records through the unchanged JEWEL
GENEVENT block and MAKEMASSLESS routine, comparing statuses exactly and other
values with relative tolerance 2e-12 / absolute tolerance 2e-13.

| Seed | Prepared events | Records | Compared values | Massless conversions | Maximum absolute error |
|---|---:|---:|---:|---:|---:|
| 2600417 | 999 | 31,539 | 315,390 | 1,502 | 4.053e-11 |
| 2600418 | 998 | 32,553 | 325,530 | 1,537 | 3.513e-11 |

All **640,920 values pass**, including 64,092 exact status decisions. There are
22,925 active status-1 records and 41,167 inactive status-4 records. All 1,997
mapped events prepare successfully; three previously known frame-mapping
failures remain in the complete attempt ledger. No tolerance was loosened.
Production coordinates, decay/evolution times, parent links and colours are
checked for preservation inside the probe.

Generated records contain PDG codes 10441 and 20443, exceeding the reference
harness's former 10,000-code limit. The harness now stores explicit code/mass
pairs; no extracted JEWEL routine changed. Nominal masses are external inputs
shared between engines, so this checks preparation arithmetic and selection,
not Pythia6/Pythia8 mass-table equivalence. Existing kinematics, massless and
preparation differential suites pass after this harness change, as do the
native input/boundary contracts.

[Machine-readable results](validation-native-preparation.json) include the
failure ledger, source manifest and input/executable hashes.

## Remaining integration

This is not a complete ISR/MPI event generator. Pythia8 source order is an
explicit candidate ordering; it has not been shown equivalent to JEWEL's
string-ordered Pythia6 record. Because massless conversion updates the hard
pair sequentially, order matters. Primary/MPI virtuality initialization,
source string ordering and full cascade integration still require validation.
No final-jet or 10% agreement claim follows from this comparison.

## Reproduce

Use fresh output paths; the probe refuses to overwrite existing outputs.

```sh
cmake --build build-pythia8 -j 4
cmake --build build -j 4
build-pythia8/jewel-native-preparation /tmp/prepared.jsonl /tmp/prepared.fixtures 1000 2600417
python3 tests/native_preparation_audit.py build/jewel-kinematics-reference /tmp/prepared.jsonl /tmp/prepared.fixtures /tmp/prepared-audit.json
ctest --test-dir build -R '^(preparation|massless|kinematics)-differential$' --output-on-failure
ctest --test-dir build-pythia8 -R '^pythia8-(input|boundary)-contracts$' --output-on-failure
```
