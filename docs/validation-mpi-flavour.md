# Native MPI outgoing-flavour channels

The experimental ISR+MPI runner now selects the initialization channel independently
for each outgoing MPI leg: GC for gluons, QQ for quarks. Its run header records
`mpi_channel_policy: outgoing-flavour`. This is an explicit Pythia8 adapter policy,
not unchanged stock GENEVENT behavior. The source-compatible default of
`initialize_imported_subsystems` remains `source_primary`, preserving inherited
TYPE1/TYPE2 for reference studies. No core splitting routine or colour-repair
rule changed.

The change resolves the quark carrying a gluon quark-pair flag documented in
[the topology boundary report](validation-native-topology.md). Primary-pair
initialization, subsystem ordering, initial pt_hat scale and transactional state
updates remain unchanged. Unsupported non-QCD pairs are still rejected before
sampling. Tests exercise both policies and assert that quark MPI legs cannot
receive a quark-pair flag under the native policy.

## Fortran comparison

The native preparation probe accepts `--initialize-native`. It emits `SEQFLAV`
fixtures. The Fortran harness sets TYPE1/TYPE2 from each pair's outgoing flavours
before calling the unchanged FINDVIRTUALITIES routine; `SEQINIT` retains the
original inherited-channel sequence. The audit requires the fixture mode to
agree with the native run header, so these policies cannot be silently mixed.
This validates the same numerical operation under the selected policy, not the
policy's equivalence to stock Pythia6 generation.

For seed 2600417, 953 supported events from 1,000 attempts pass: 6,356 pairs,
1,483,924 values, no discrete-field or random-draw mismatches. Maximum absolute
error is 1.21076e-11; each value passes abs=2e-13, rel=2e-12. The maximum pair
four-momentum residual is 1.05327e-6 GeV in both engines. Source-compatible hard
initialization contracts and differential tests also pass. The three selected
Pythia8 adapter/integration contract suites pass.

## Hadron samples

| Setting | Attempts | Hadronized | Junction events | Selected jets |
|---|---:|---:|---:|---:|
| Vacuum | 1,000 | 953 | 319 | 1,414 |
| Expanding medium | 1,000 | 953 | 319 | 5,046 |

Both runs use seed 2600417. The 47 rejected attempts consist of 32 hard-parton
junction colour-partner cases, 14 non-QCD subsystem cases and one ISR frame
reconstruction failure. No supported event fails cascade evolution or
hadronization in these samples. Rejections and their weights remain in the ledger.
The corresponding 20-attempt smoke samples both accept all 20 events.

The streaming `tools/audit_native_topology.py` independently verifies unchanged
physical-record order, IDs, flavours, momenta, masses, colours and production
vertices at the boundary, junction colour balance and attempt/weight accounting.
All accepted events pass the existing hadronizer conservation criterion:
absolute component residual <= 1e-8 * max(1 GeV, total input energy).
Maximum component residuals are 2.72299e-7 GeV (vacuum) and 2.72300e-7 GeV
(medium); maximum residual/energy is below 5.5e-11. The tighter small-sample
component diagnostic (abs=1e-7 GeV, rel=1e-9) has five vacuum and six medium
component excursions, retained in the report. No production tolerance changed.

The [interactive hadron browser](mpi-flavour-hadron-browser.html) offers jet pT,
mass, girth, pTD and Soft Drop distributions, settings and CSV export. Analysis
uses hepyy/cppyy, anti-kt R=0.4, pT>=10 GeV and |eta|<2 after full-phase-space
clustering. Medium recoils remain included without thermal subtraction, so the
larger medium jet count is not a physical enhancement claim. These native
samples exceed 1,000 selected jets each; the requested matched Fortran/native
ratio gate is still outstanding.

[Machine-readable evidence](validation-mpi-flavour.json) contains reference
results, hashes, boundary audits and residual excursions. Remaining work includes
junction-connected hard initialization, non-QCD channels, frame reconstruction,
full configuration matching and the Fortran/native observable comparisons.

## Reproduction

```sh
build-pythia8/jewel-native-preparation native.jsonl native.fixtures 1000 2600417 --initialize-native
python tests/subsystem_reference_audit.py build/jewel-medium-reference native.jsonl native.fixtures.initialization reference.json
build-pythia8/jewel-run-events vacuum.jsonl 1000 2600417 vacuum --experimental-isr-mpi
build-pythia8/jewel-run-events medium.jsonl 1000 2600417 medium --experimental-isr-mpi
python tools/audit_native_topology.py vacuum.jsonl vacuum-audit.json
python tools/audit_native_topology.py medium.jsonl medium-audit.json
# In the hepyy/cppyy environment with hepyy-utils installed:
python tools/analyze_native_events.py vacuum.jsonl medium.jsonl --output jets.json --generator build-pythia8/jewel-run-events
python tools/render_native_report.py jets.json browser.html
```
