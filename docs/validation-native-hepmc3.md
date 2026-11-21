# Native final-state HepMC3 export

`tools/export_native_hepmc3.py` exports a completed native hadronized JSON run to
HepMC3 ASCII through hepyy's native HepMC3 C++ binding. It runs after generation;
the standalone generator itself gains no Python dependency.

```sh
# In the hepyy/cppyy/hepbundle environment:
python tools/export_native_hepmc3.py native.jsonl native.hepmc3
python tests/native_hepmc3_roundtrip.py native.jsonl native.hepmc3 roundtrip.json
```

The exporter selects the same particle collection as the common analysis:
projected particles when on-shell projection is enabled, otherwise subtracted
particles when subtraction is enabled, otherwise final hadronized particles.
It preserves all final particles, including neutrinos; analysis selections are
not imposed during export. All are written with HepMC status 1, GEV/MM units,
explicit generated mass, original momentum and a native-particle-ID attribute.
The file is final-state interchange, not a reconstruction of beams, production
vertices or shower ancestry. No vertex is invented for the exported particles.

HepMC event numbers preserve original native attempt IDs, including gaps for
failed attempts. One unmodified nominal event weight is written per accepted
event. Run attributes retain the entire native run header and summary, the
source SHA-256, chosen particle collection and normalization policy. Event
attributes retain geometry, initial hard-leg snapshots, impact parameter,
centrality and lifetime when available.

## Normalization and reader limits

Failed events are not written as empty physical events. Their counts and weights
remain in the run ledger. **Do not normalize these files by the number of written
events as if failed attempts never occurred.** The native summary is retained in
`jewel_native_summary`; hard-generator cross section and total generator weight
are there, together with accepted/failed weight sums. A GenCrossSection is not
assigned because a hard-generator cross section is not automatically the cross
section of the surviving output sample. The general matched-normalization audit
remains outstanding.

The existing hepyy-utils ROOT converter can read the final particles, event
numbers and weights. It does not automatically promote these custom run/geometry
attributes into its standard ROOT tables. Retain the HepMC/native JSON ledger
for normalization, and use the native common analysis for the new geometry
association. This is not yet the custom Fortran CSV-sidecar interface or full
history/thermal-reference HepMC export. Already-subtracted output must not be
subtracted again by a downstream converter.

## File integrity

Before writing, the exporter verifies a complete header/summary, contiguous
attempt IDs, all outcome counts, total/accepted/failed weight sums, finite final
momenta/masses and unique final particle IDs. It streams the file again to write
and verifies the source hash did not change. Output is published atomically
without replacing an existing destination. Invalid input or a failed write
removes only the temporary file.

## Validation

The raw medium sample round-trips **18 accepted events, 4,920 final particles
and 18 geometry records exactly**, including weights, attempt IDs, masses,
particle IDs and all retained attributes. The native run still records 20
attempts. [Evidence](validation-native-hepmc3.json).

Three additional samples each round-trip **19 events and 3,065 particles**
exactly: subtraction without projection, projection mode 1 and projection mode 2.
[Collection-policy evidence](validation-native-hepmc3-policies.json).

The existing hepyy-utils 0.3.0 converter reads the raw export into 18 event-info
rows and 4,920 tracks. Weights and attempt IDs agree exactly; every momentum
component agrees after conversion to the ROOT branch's storage precision.
The independent reader check used pyhepmc 2.16.1 and uproot 5.7.7.
[Compatibility evidence](validation-hepmc3-utils.json). The missing pyhepmc reader
was installed only into a temporary test directory, not the working environment.

Four standard-library contract tests cover malformed/incomplete inputs, outcome
and weight ledgers, collection policy and preservation of an existing output:

```sh
ctest --test-dir build -R '^native-hepmc3-contracts$' --output-on-failure
# Requires hepyy-utils and its pyhepmc/uproot conversion dependencies:
python tests/hepmc3_utils_compatibility.py native.jsonl native.hepmc3 compatibility.json
```

These are output-preservation checks, not evidence of Fortran/native physics
agreement or completion of the matched 1,000-jet comparison.
