# Vacuum string construction and native hadronized runner

Recorded 2026-11-21 on macOS arm64, AppleClang 21, GNU Fortran 16.2 and Pythia8
8.317. This is a reduced native generator checkpoint, not a claim of full JEWEL
2.6.0 equivalence or validated jet spectra.

## Source routine comparison

`make_vacuum_strings()` translates full MAKESTRINGS_VAC and CLEANUP, with the
previously validated COMPRESSEVENT used when requested. The reference extracts
the unchanged routines and ISDIQUARK from the pinned stock source. PEVREC is
also unchanged, including its V=MV mutation on the missing-partner error path;
only PYLIST's text is replaced by the existing diagnostic stub, now accepting
both listing modes 2 and 3.

[292 cases and 1,120,328 output values agree exactly](validation-strings.json).
Cases cover quark/gluon/diquark chains, ordering, multiple strings, active
statuses 1/3/4/5, colourless active records, missing partners, inactive partners,
closed gluon loops, compression on/off and capacity limits. They include all
18 accepted events from the native medium smoke sample, each with compression
on and off. The set has 64 expected discarded results and six empty successful
results; these are classified source outcomes, not hadronized events.

```sh
ctest --test-dir build -R 'strings|compression-differential' --output-on-failure
python3 tests/strings_differential.py build/jewel-strings-probe \
  build/jewel-strings-reference native-strings.json --sample medium-partons.jsonl
```

The source has two significant record semantics:

- CLEANUP moves K/P/V and erases parents, but leaves colour and medium metadata
  in their physical slots. The boundary routine preserves this exactly and
  returns a separate mapping to original input slots.
- Its pure-gluon remainder scan can leave closed loops unconsumed, then remove
  them during cleanup. The routine reports those original slots explicitly.
  The Pythia8 adapter rejects such a result rather than silently losing momentum.

This implements HADROTYPE=0. Minimum-length strings (HADROTYPE=1) and junction
interpretation are still pending.

## Native hadronization boundary

`prepare_vacuum_hadronization()` uses the returned source-slot mapping to retain
stable native IDs, actual colour tags and production coordinates. It converts
fm to mm explicitly. It rejects unfinished cascades, discarded string builds,
unconsumed partons, and colour tags without exactly one colour and one anticolour
endpoint. Optional recoil exclusion follows GENEVENT's status-3 to status-22
change before string construction; it does not invent replacement endpoints.
The native event itself is unchanged. Tests exercise compression with a removed
ancestor, source mapping, colour preservation, unit conversion and a separate
Pythia8 hadronization call.

No momentum projection or fragmentation retuning was added. The hadronizer
retains its existing on-shell checks, disabled native showers/reconnection,
explicit decay settings, charge closure and momentum closure. JEWEL's
PUTPARTICLESONSHELL is an optional post-subtraction operation in GENEVENT, not
a routine to apply indiscriminately before hadronization.

## End-to-end development executable

```sh
build-pythia8/jewel-run-events vacuum-hadrons.jsonl 20 2601001 vacuum
build-pythia8/jewel-run-events medium-hadrons.jsonl 20 2601001 medium
python3 tools/check_parton_run.py vacuum-hadrons.jsonl medium-hadrons.jsonl \
  --output hadron-audit.json
```

The optional fifth argument is a new replay directory, as for the parton runner.
The executable uses the same reduced PPJJ settings: pp at 5020 GeV, hard pT
20–100 GeV, ISR/MPI off, angular ordering off, coherence on, recoil rescattering
off, longitudinal scatterer boost off, b=0 and the simple expanding medium.
The hadronizer is a separate engine with the supplied seed and recorded settings.

| Setting | Attempts | Hadronized events | Final particles | Failures |
|---|---:|---:|---:|---:|
| Vacuum | 20 | 20 | 2,199 | 0 |
| Medium | 20 | 18 | 4,779 | 2 cascade failures |

Every accepted cascade in this sample passes string construction and
hadronization. All final particles are colourless. The maximum component of
input-to-final hadronization momentum difference is 1.24283e-8 GeV in both
samples, below the unchanged hadronizer conservation tolerance. Native code
also checks charge closure. The JSON audit checks finite values, source IDs,
finished cascade records, attempt/weight accounting and momentum closure.
[Recorded sample audit](validation-native-hadrons.json).

`jewel-hadron-development-v1` JSONL includes native particles/archived ancestry,
thermal references, ordered `hadronization_input`, `hadronized_particles` and
`hadronization_sources`. The last array maps imported Pythia record entries
1..N to original native IDs; captured particle IDs are Pythia indices plus one.
Failures retain their stage/reason/weight. The existing `evolution_failures`
summary key counts all processing failures after hard generation, including
string/hadronization failures if present; inspect each failure's stage.

This is not yet HepMC output. The sample is too small for the requested
1,000-selected-jet comparison and is not a Pythia6/Pythia8 fragmentation
comparison. Known cascade replay differences, other production channels,
ISR/MPI, minimum-length strings and custom subtraction/output features remain
open. Full generator completion and the 10% physics gate remain unproven.
