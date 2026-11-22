# Native thermal-subtraction integration

Recorded 2026-11-21. The hadronized runner now accepts `--subtraction-radius R`
for finite R ≥ 0. Omission leaves the existing output unchanged. Parton-only runs
and combinations with `--keep-recoils off` reject before creating output.

```sh
build-pythia8/jewel-run-events medium-subtracted.jsonl 20 2601001 medium \
  --centrality 0 10 --table-energy jewel --longitudinal-boost on \
  --angular-ordering 1 --subtraction-radius .4
python tools/analyze_native_events.py medium-subtracted.jsonl --output analysis.json
python tools/render_native_report.py analysis.json report.html
```

Use the hepyy/cppyy environment for analysis. No standalone SWIG FastJet binding
is used. Analysis clusters the complete selected particle collection, then applies
jet cuts. Neutrinos are excluded for jet finding after subtraction, not before.

## Adapter and output contract

The adapter passes all Pythia8 final particles and all used thermal-ledger centre
momenta to the validated kernel. It uses zero process offset, not the broken
upstream wrapper's two-entry/double-offset behaviour. Thermal dummies are not
substituted for the original thermal momenta.

Raw `hadronized_particles`, shower records, thermal references and hadronization
provenance remain present and unchanged. The new `subtracted_particles` collection
has its own sequential IDs, original hadron-record `source_id` values, PDG IDs,
four-momenta, mass metadata and explicit `thermal_residual` tags. Residual thermal
entries retain the source kernel's positive momenta and random pion IDs, with
source ID zero. Their metadata does not imply physical charged-pion on-shell mass.

The run header records algorithm `jewel260-kernel-zero-offset`, radius, all-final
input selection, stored-centre thermal selection, mass policy, and `put_on_shell=0`.
Random pion IDs use an independent SplitMix64 stream with event seed
`run_seed + 2000000 + 100*attempt`. Each event records this seed and draw count,
plus skipped-input and spacelike-mass counts. This is a recorded native random
policy, not an attempt to reproduce the upstream global C-library RNG sequence.

`tools/analyze_native_events.py` requires `subtracted_particles` when the header
declares subtraction; it never silently falls back to raw hadrons. Otherwise it
uses raw hadrons as before. The generated HTML labels raw/subtracted settings
and displays the policy in provenance.

For the [ATLAS example](atlas-soft-drop-comparison.html), `--compact-hadrons`
keeps the raw and subtracted final-state collections with PDG IDs, four-momenta
and the residual thermal flag. The full event format retains the thermal ledger,
IDs, mass fields and diagnostics. Compact subtraction supports the direct
kernel output with `put_on_shell=0`. Pass `--exclude-thermal-residuals` for
the subtracted jet calculation and `--raw-hadrons` for its same-event diagnostic.

## Signed mass metadata, not a numerical clamp

Real hadronized records expose a source issue not covered by the original
massive synthetic fixtures. In a saved event, the unchanged reference helper
emits NaN mass fields for 30 particles while all their four-momenta remain finite.
Some inputs are already slightly spacelike from floating-point generation and
boosts. The native runner uses `SignedMass`: negative invariant squares are
represented by `-sqrt(-m²)` and counted in `subtraction_spacelike_mass_fields`.
This preserves information rather than selecting a roundoff tolerance to hide it.
**No momentum is clamped, projected or put on shell.**

The kernel's default `Reference` mode remains strict and rejects non-real mass
results. Native contracts cover both a near-lightlike and a substantially
spacelike input, verifying signed metadata and unchanged momenta in the explicit
signed mode. Nonfinite kinematics still fail normally.

## Validation

The integration test generates a raw event and two identical subtracted runs.
It verifies all existing event fields and the summary are identical, repeated
subtracted output is deterministic, source IDs identify final hadrons, residual
IDs are valid pions, draw counts equal residual counts, and invalid combinations
fail before output creation.

The same actual input particles/thermals are supplied to the original helpers:
298 output particles agree, with maximum four-momentum difference
2.77556e-17. All finite reference mass fields agree at abs=2e-13 / rel=2e-12;
the 30 non-real reference masses correspond to explicitly signed native fields.
There are 31 residual thermal particles. Their random PDG IDs are not compared
across the different RNG policies. The 132-case strict kernel comparison and
native contracts continue to pass.

Two fresh 20-attempt runs from the same executable accept 19 events each and
have identical pre-subtraction fields. Both retain the known event-14 cascade
failure in their normalization. hepyy selects nine raw jets and five subtracted
jets using anti-kt R=0.4, pT ≥10 GeV and |eta|<2.

[Browsable smoke report](native-subtraction-smoke.html) ·
[Machine-readable evidence](validation-native-subtraction.json)

The report's JavaScript passes a syntax check; it has not been visually reviewed.
These are tiny native-only integration samples, not the required ≥1000-jet
Fortran/native ratios. Full upstream-wrapper compatibility, `PUTONSHELL` modes,
remaining generator features and final physics acceptance remain incomplete.

```sh
ctest --test-dir build-pythia8 -R 'subtraction-contracts' --output-on-failure
python3 tests/runner_subtraction.py build-pythia8/jewel-run-events \
  build-pythia8/jewel-run-partons build/jewel-subtraction-reference
```
