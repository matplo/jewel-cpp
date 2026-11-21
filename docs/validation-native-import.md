# Native import of Pythia8 final records and subsystem pairs

`subsystem_hard_final_pairs` follows the existing reciprocal copy-chain walk
from each subsystem's two original outgoing process records to their final
copies. It does not rank particles by momentum. The primary pair must match
`HardEventSnapshot::hard_final`; MPI pairs retain their subsystem identity.
During input capture, all subsystem pairs are additionally checked against
Pythia8's own `iBotCopy()` navigation, extending the earlier primary/ISR checks.

`import_pythia_final_record` creates an `ImportedPythiaRecord` containing every
final particle, the source/native ID map, particle roles, all subsystem hard
pairs and pThat values, the event weight and copied junction metadata. Roles
are primary hard, MPI hard, ISR emission and remnant. The validated input
subsystem ledger already partitions scattering/emission outputs; remaining
finals are classified as remnants in this FSR/hadronization-off adapter.

The importer creates one scratch record and one inert process anchor per
subsystem. Each hard pair has reciprocal links to its process anchor.
All physical particles start at **inactive status 4**. Their momenta, recorded
masses and colours are copied exactly. Auxiliary Pythia vertices are converted
from mm to fm, while JEWEL production coordinates initialize independently at
`(x,y,0,0)` and decay times at the supplied lifetime. Evolution time starts at
-1e6. ISR timing can then be attached using the existing stable-ID helper.

The optional import order must be a complete permutation of final source IDs;
duplicates and omissions are rejected. The default is source record order.
No random stream is used, and the function returns a new validated state rather
than modifying an existing one in place.

## Explicitly incomplete evolution state

This is a raw import stage, **not an evolution-ready event**. ISR and remnant
parent links remain zero/unresolved; their original source IDs and roles are
retained for the next ancestry step. No coherence parentage is invented, no
parton is activated, and no virtualities are initialized. Process anchors are
bookkeeping records, not physical Pythia scattering particles. The native
runner's ISR/MPI guard remains in place. Source string ordering, unresolved
parents, selection, hard/MPI virtuality initialization and cascade integration
still need to be connected and validated.

## Generated-record audit

The timing probe now uses the importer with reversed source order, then attaches
ISR times. Its JSON includes each subsystem's source/native hard pair, anchor
count, role counts and native weight. An independent audit follows the captured
snapshot's daughter chains, reconstructs expected native IDs, and compares all
hard-pair IDs, role counts, weights and ISR attachments. The Pythia8 copy-walk
cross-check runs during generation as well.

| Seed | Mapped events | Primary hard | MPI hard | ISR emissions | Remnants |
|---|---:|---:|---:|---:|---:|
| 2600417 | 999 | 1,998 | 11,530 | 13,249 | 4,762 |
| 2600418 | 998 | 1,996 | 11,966 | 13,683 | 4,908 |

All **64,092 final particles** are accounted for. All **27,490 hard-leg ID
associations** and **26,932 ISR native attachments** match the source snapshots.
The unchanged Fortran replay still passes all 134,660 emitted timing values
with exact equality, and both timing fixture files are byte-identical to the
previous timing-only run. The three previously recorded frame failures remain
visible and unchanged; no acceptance tolerance changed.

Generated-event contracts verify hard-pair parent/daughter links, primary/MPI
roles, source/native maps, weights, junction contents, auxiliary-vertex units,
independent production coordinates, inactive statuses and invalid import-order
rejection. Existing attachment and adapter-boundary checks also pass. Both
selected CTest suites pass.

[Machine-readable evidence](validation-native-import.json) includes role totals,
complete audit reports, failure records and hashes. The sample uses the same
two-seed ISR+MPI configuration and fixed timing tape as
[the mapped timing validation](validation-mapped-isr-timing.md); it is not a
JEWEL cascade or a final-jet comparison.

Reproduce using the [timing-probe commands](validation-mapped-isr-timing.md#reproduce)
with fresh output paths, followed by:

```sh
ctest --test-dir build-pythia8 \
  -R '^pythia8-(input|boundary)-contracts$' --output-on-failure
```
