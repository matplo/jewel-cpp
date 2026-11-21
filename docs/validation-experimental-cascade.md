# Experimental ISR/MPI parton cascade integration

`jewel-run-partons --experimental-isr-mpi` now connects source capture, mapped
ISR timing, survivor preparation and primary/MPI initialization to the native
cascade. It uses the event's sampled production point and lifetime, with the
same event RNG continuing through timing, initialization and evolution.
The default runner remains ISR/MPI-off. The option currently rejects the
hadronized runner: junction/string topology is not yet ready for that path.

Every attempted event is retained in the existing weighted output ledger,
including mapping, preparation, initialization and cascade failures. Replay
fixtures export scheduling links (including MPI parent zero), independently
of retained stable provenance. The output run header identifies the mode as
experimental and not fully validated physics.

## Smoke samples and reference replay

Twenty attempts with seed 2600417, PP 5020 GeV, pThat 20–100 GeV, ISR+MPI on,
first-angle ordering off, angular-ordering mode 0, recoils on, rescattering off,
no longitudinal boost and 100 GeV table energy were run in vacuum and medium.
Vacuum disables scattering rates; it uses the runner's existing medium-derived
lifetime convention. These settings are not claimed matched to stock card-level
generation.

The vacuum sample completes all 20 cascades and retains weight 20. The medium
sample completes only attempts 1 and 14: 17 attempts fail with nonfinite shower
records and one with nonfinite kinematics. Accepted weight is 2, failed weight
18, generator weight sum 20. This failure rate is a substantial open problem;
the two completed events are not representative validation of medium physics.

The exact initialized records and random cursors are replayed in the existing
C++ and guarded Fortran cascade probes. The Fortran scheduler is bounded harness
code around unchanged source evolution routines; timeout/nonfinite outcomes
are never counted as numerical parity. Strict comparison uses relative 2e-12,
absolute 2e-13, and exact discrete fields.

The complete vacuum replay compares 93,311 values. Eleven events pass; nine
have 53 total scalar mismatches at the unchanged strict tolerances. All 20
execution headers and random-draw counts match. Thus native completion of all
20 events is not reported as strict numerical agreement of all 20.

[Machine-readable evidence](validation-experimental-cascade.json) includes both
sample summaries, complete failure ledgers and all replay outcomes.

For the selected medium attempts:

- Attempt 14 passes 7,045 values, including exact draw count 1,077,377.
- Attempt 1 has matching execution counts and draw count 108,454, but 83
  mismatches among 11,922 values. These include numerical differences and an
  angular-required flag mismatch; this is not a pass.
- Attempt 2 reaches nonfinite states in both implementations at draw 6,052,
  with different partially updated records; this is not a pass.

The experimental CLI contract verifies enabled generator settings, a complete
ledger, finished native records, exact native replay and rejected repeated or
hadronized-mode options. The existing flow/default-compatibility contract also
passes. Full jet comparisons, junction/non-QCD handling, frame failures,
source ordering and numerical cascade failures remain open.

## Reproduce

Use new output paths and new replay directories.

```sh
cmake --build build-pythia8 -j 4
build-pythia8/jewel-run-partons /tmp/isr-mpi-vac.jsonl 20 2600417 vacuum /tmp/isr-mpi-vac-replay --experimental-isr-mpi
build-pythia8/jewel-run-partons /tmp/isr-mpi-med.jsonl 20 2600417 medium /tmp/isr-mpi-med-replay --experimental-isr-mpi
python3 tools/replay_native_cascade.py build/jewel-medium-probe build/jewel-medium-reference /tmp/isr-mpi-vac-replay /tmp/isr-mpi-vac-comparison --timeout 60
python3 tools/replay_native_cascade.py build/jewel-medium-probe build/jewel-medium-reference /tmp/isr-mpi-med-replay /tmp/isr-mpi-med-comparison --attempts 1 2 14 --timeout 60
ctest --test-dir build-pythia8 -R '^(experimental-isr-mpi-contracts|runner-flow-contracts)$' --output-on-failure
```

## Follow-up: imported fraction correction

The original 2/20 medium result above used an incorrect imported fraction
default. The [source event-start correction](validation-imported-fractions.md)
raises completion to 16/20 for the same sample. Vacuum output is unchanged.
The original results remain historical evidence, not the current failure rate.

A subsequent [zero-CM-momentum roundoff correction](validation-zero-cm-limit.md)
raises the same medium sample to 20/20 completion. This deliberately resolves
a source-nonfinite algebraic limit; it does not convert historical Fortran
failures into parity passes or establish jet-level agreement.
