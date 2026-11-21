# Native on-shell projection integration

Recorded 2026-11-21. `jewel-run-events` accepts `--put-on-shell 0|1|2` together
with `--subtraction-radius R`. Omitted projection and explicit mode 0 produce
identical output. Projection without subtraction, invalid/repeated modes and
parton-only runs reject before output creation.

```sh
build-pythia8/jewel-run-events projected.jsonl 20 2601001 medium \
  --centrality 0 10 --table-energy jewel --longitudinal-boost on \
  --angular-ordering 1 --subtraction-radius .4 --put-on-shell 2
python tools/analyze_native_events.py projected.jsonl --output analysis.json
python tools/render_native_report.py analysis.json report.html
```

## Masses, range and records

Nominal masses come from the actual hadronizer's `Pythia8::ParticleData::m0`,
without random resonance-mass sampling. Unknown IDs and invalid nominal masses
throw. The header records this source, the Pythia version and the projection mode.
Every projected particle records its nominal mass. This is a native mass policy;
Pythia6/Pythia8 mass-table equivalence is not implied.

The reduced PPJJ runner projects the complete subtracted collection using the
explicit safe range starting at C++ index zero. It does not reproduce the stock
routine's invalid Fortran array-index-zero access. Other channels and their
range/bypass rules still require separate adapters.

Raw hadrons and `subtracted_particles` remain unchanged. Enabled projection adds
`projected_particles`, retaining IDs, source IDs, PDG IDs and thermal-residual
flags. Analysis requires this collection whenever the run declares mode 1 or 2;
it otherwise uses the subtracted/raw collection as before. Missing projected
records do not silently fall back to another collection.

Each event records total four-momentum before and after projection, iteration
counts, convergence flags and insufficient-energy status. For mode 1, iteration
counts are zero and the unused convergence flags are false: no iterative step
was requested. Mode 2 exposes the source's nonconverged or insufficient-energy
outcomes without silently relabelling them as convergence. See the
[core behaviour](validation-projection.md) for why total energy need not be
conserved on these paths. Projection changes no shower, subtraction or
hadronization random draws.

## Reference and integration results

The integration test verifies preserved upstream records, deterministic mode-0
behaviour, exact spatial-momentum preservation in mode 1, nominal photon/pion
masses, on-shell final records, metadata totals and invalid-option handling.
Fortran receives the actual preprojection records and the **same nominal masses**.
Its unchanged routine is exercised over the valid selected range, as documented
in the component harness.

For the first fixed-b event (298 particles / 1,490 values per mode): mode 1 agrees
exactly; mode 2 differs by at most 4.33681e-19.

Three fresh centrality-sampled runs, each with 20 attempts from the same binary,
accept the same 19 events. Every preprojection event field is identical across
modes. All 19 accepted events were independently replayed through Fortran:

| Mode | Compared scalars | Maximum absolute difference | Mismatches |
|---|---:|---:|---:|
| 1 | 15,325 | 0 | 0 |
| 2 | 15,325 | 2.27374e-13 | 0 |

Tolerance remains abs=2e-13 / rel=2e-12. Every sampled mode-2 event converges,
with no insufficient-energy outcome. Maximum relative energy change is
6.85717e-4 for mode 1 and 3.03563e-9 for mode 2. The known event-14 cascade
failure is retained in every sample's normalization.

hepyy/cppyy anti-kt R=0.4 clustering, followed by pT ≥10 GeV and |eta|<2 cuts,
selects five jets in each of modes 0, 1 and 2. Equal tiny counts are not physics
validation. [Selectable smoke report](native-projection-smoke.html) ·
[Reference comparisons and provenance](validation-native-projection.json).
The HTML passes JavaScript syntax checking but has not been visually reviewed.

Reproduce a complete accepted-event operation comparison:

```sh
python3 tools/validate_native_projection.py projected.jsonl \
  build/jewel-projection-reference projection-validation.json
python3 tests/runner_projection.py build-pythia8/jewel-run-events \
  build-pythia8/jewel-run-partons build/jewel-projection-reference
```

Full generator coverage, original particle-data matching, custom exports and
≥1000-jet Fortran/native spectrum/substructure acceptance remain outstanding.
