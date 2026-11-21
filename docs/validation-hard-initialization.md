# Hard-dijet initialization and native cascade checkpoint

Validated on 2026-11-21, macOS arm64, AppleClang 21 and GNU Fortran 16.2,
Release builds. This checkpoint connects reduced Pythia8 hard events to the
native cascade. It does not establish full-generator equivalence.

## Unchanged Fortran reference

`initialize_hard_dijet()` ports the PPJJ branch of FINDVIRTUALITIES, including
coupled two-body reshaping, virtuality retries, energy-fraction and optional
first-angle vetoes, and production/decay initialization. `colour_opening_angle()`
ports GETTHETAM. The reference extracts the complete unchanged routines from the
hash-pinned stock JEWEL 2.6.0 file; only the PPJJ branch is exercised here.
MPI, EEJJ and boson/photon production branches are not implemented by this API.

The [machine-readable comparison](validation-hard-initialization.json) contains
288 cases and 45,792 output values. All pass at relative tolerance 2e-12 and
absolute tolerance 2e-13; discrete values and random-draw counts agree exactly.
The largest absolute difference is 9.3223e-12 on a large value, within the
relative tolerance. Ninety-six cases require retries, with at most 15 trials
and 598 random draws. Cases vary flavour, pair boost, virtuality scale,
first-angle ordering and one frozen leg. Configuration and executable hashes
are recorded in the report.

The port preserves the otherwise redundant weight draw, the source's default
REAL formation-time constant, and repeated boost arithmetic. Failed calls leave
the caller's event unchanged; consumed random draws are not rewound.

```sh
cmake --build build -j4
ctest --test-dir build -R hard-initialization --output-on-failure
```

## Native Pythia8 integration

`prepare_pythia_dijet()` imports final hard copies and frozen beam remnants,
retaining a native-ID to Pythia-ID mapping. A synthetic inactive process node
separates the hard collision from a shower splitting while allowing later
shower daughters to undergo coherent evolution. This is not a mapping of the
complete incoming Pythia ancestry. The adapter rejects first-angle ordering;
the standalone core routine supports it when supplied complete colour ancestry.
The hard scale comes from the snapshot. Pythia vertices are converted from mm
to fm before import; initialized hard vertices use sampled medium coordinates.

Build with the Pythia8 options in [boundary validation](validation-pythia8.md).
The new executable uses native C++ throughout:

```sh
build-pythia8/jewel-run-partons vacuum.jsonl 20 2601001 vacuum
build-pythia8/jewel-run-partons medium.jsonl 20 2601001 medium
python3 tools/check_parton_run.py vacuum.jsonl medium.jsonl --output audit.json
```

These are fixed development settings: pp at 5020 GeV, hard pT 20–100 GeV,
ISR/MPI off, no hadronization, angular ordering off, coherence on, recoil
rescattering off, longitudinal scatterer boost off, and compression enabled.
The expanding medium uses A=208, b=0, initial time 0.4 fm, initial temperature
0.59 GeV, cutoff 0.17 GeV and maximum rapidity 2. Other medium parameters use
`MediumConfig` defaults. Vacuum uses zero scattering cross-section scale;
geometry and initialization still use the same model and random-seed convention.
These settings are not a production JEWEL tune.

The [recorded smoke-test audit](validation-native-partons.json) reports:

| Setting | Attempts | Accepted | Cascade failures | Final records |
|---|---:|---:|---:|---:|
| Vacuum | 20 | 20 | 0 | 322 |
| Expanding medium | 20 | 16 | 4 | 1,172 |

Two medium attempts fail with nonfinite kinematics input; two fail with an
undefined scattering invariant for radiation probability. Their attempt IDs,
weights, stages and random-draw counts remain in the JSONL output. No failed
attempt is silently regenerated. Final-record counts include recoils and frozen
remnants; they are not selected-jet counts. Accepted events pass finite-number,
ancestry, finished-status and run-weight accounting audits. These are integrity
checks, not a comparison of complete Pythia-initialized events against Fortran.

The medium, sampling, radiation, final-splitting, PDF, scattering and
final-scattering reference tests were rerun alongside hard initialization:
all 10 selected CTest checks passed. All 22 native C++/Pythia8 contract tests
also passed. Existing expanded-cascade discrepancies
remain open; see [scheduler progress](scheduler-progress.md).

Next validation must replay identical imported hard states through both engines
and diagnose these medium failures before physics comparisons. Full colour-string
construction, hadronization after the JEWEL cascade, production channels,
ISR/MPI and custom outputs remain incomplete. The 1,000-selected-jet/10% gate
has not been met.
