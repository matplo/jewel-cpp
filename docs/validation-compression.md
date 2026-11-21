# Event-record compression: boundary routine

`compress_records` in `jewel/compression.hpp` ports stock COMPRESSEVENT at a
transient generator-record boundary. The native adapter is now available through
`ShowerState::compress()` and the opt-in `CascadeConfig::compress_records` flag.
The transient boundary array is not the persistent shower storage.

The Release comparison against the unchanged, hash-verified JEWEL 2.6.0 routine
passes **222 cases and 1,531,146 values exactly**, including inactive tail slots.
[Machine-readable results](validation-compression.json) record input, source and
executable hashes. Tests include each removable status, protected selection,
reserved first slot, no removable records, the ten/eleven-removal threshold,
random mixed records and full 23,000-slot inputs. The source is extracted from
the frozen stock file without changing its body.

## Preserved source behavior

- Physical slot zero is never removed. Status 11, 12, 13 and 14 records are
  removed except for the selected protected record.
- Retained records keep order. The selected slot is updated when moved.
- Genealogy fields are zeroed only on moved records. The prefix before the
  first removed slot is unchanged.
- Momentum, auxiliary vertex, production point, colour and angular metadata
  move with the record. Evolution times and medium-induced flags remain at
  their original physical slots: the source routine does not access them.
- The inactive tail is cleared except for evolution times, medium-induced flags
  and angular-required flags. Source COMPRESSEVENT does not clear AOREQ there.
- The returned room flag means **more than ten records removed**. It can be
  false after successfully removing up to ten records.

The C++ result also returns retained source-slot indices to support stable-ID
mapping. The active count is separate from physical vector size so repeated
compression does not reactivate cleared tail records. No-selection uses the
input active count as a sentinel; that sentinel is retained in the result,
matching the probe's representation of source L1=0. On another call, pass the
new active count as the no-selection sentinel. Contracts cover these rules
and invalid bounds.

## Native scheduler integration

`ShowerState::compress(protected_id)` performs a transactional compaction. Active
records retain stable IDs; removed records move into an ancestry archive.
`particle.parents`, `parent`, `collision_source` and `coherent_partner` continue
to describe physical history, and `at(id)` can retrieve archived ancestors.
Snapshots preserve the archive and source scheduling links. New IDs are never
recycled, and archived cycles are rejected during restoration.

Scheduling uses a separate source-slot view through `scheduling_parent()` and
`scheduling_daughters()`. Coherent evolution, initial scattering and angular
ordering consult this view, so retaining ancestry does not restore a source
link that COMPRESSEVENT cleared. Newly constructed daughters get fresh links;
rollback restores the prior scheduling links. If a source link points outside
the active record, C++ rejects the read explicitly rather than guessing an
ancestor. Further pathological-prefix coverage remains necessary.

Set `CascadeConfig::compress_records=true` to compress when the active record
exceeds 20,000 slots. As in MAKEBRANCH, time initialization precedes compaction;
visit order continues using physical slots. The flag is opt-in while broader
validation remains open. The source default is COMPRESS=true, which future
run-card initialization must select explicitly.

## Compression-enabled cascade comparisons

| Fixture | Compared values | Maximum absolute difference | Result |
|---|---:|---:|---|
| Four vacuum cascades with 20,000 removable padding records | 10,608 | 3.28e-14 | Pass |
| Exact initial 20,000-record threshold, then growth/compression | 2,679 | 3.28e-14 | Pass |
| Medium cascade with coherence enabled and padding | 3,246 | 1.48e-13 | Pass |
| Rotated medium cascade, seed 1260555, with padding | 3,192 | 2.74e-12 | Four stored-transfer mismatches |

All comparisons use the existing 2e-12 relative / 2e-13 absolute scalar tolerance,
with exact discrete fields and random-draw counts. No tolerance was widened.
Reports: [vacuum](cascade-compression.json),
[threshold](cascade-compression-boundary.json),
[coherence-enabled medium](cascade-compression-coherent.json),
[remaining transfer differences](cascade-compression-medium.json).
Padding is a controlled capacity fixture, not a physical collision sample.
Coherence enabled does not imply that a coherent merge occurs after compression:
the source deliberately clears the moved records' scheduling ancestry.

The threshold case exposed an important PEVREC side effect. Compression can
leave an evolution time earlier than the moved record's production time.
MAKEBRANCH prints a timing diagnostic and calls PEVREC, which copies every
active MV production/decay record into V before listing the event. The C++
scheduler now performs the same auxiliary-vertex synchronization. The reference
extracts PEVREC unchanged; only its PYLIST textual output is replaced with an
explicit stderr marker. The old STOP-72 diagnostic stub has been removed.
The source diagnostic is retained in the threshold test's stderr artifact.

Native contracts cover preserved ancestry, source-link clearing, physical-slot
metadata, subsequent appends, repeated compaction and snapshot restoration.
They do not yet establish compression parity for every angular-ordering mode,
recoil/ISR topology or unusual prefix genealogy. The full-generator acceptance
goal remains incomplete.

Run the focused checks with:

```sh
ctest --test-dir build -R 'compression' --output-on-failure
```

Historical full-record boundary results above remain exact. The rotated medium
fixture is kept as an explicit failing diagnostic outside the passing CTest
set, alongside the pre-existing expanded cascade replay failures.

Final checks: the broader regression run passed 48/48 tests in 162.64 seconds,
excluding the pre-existing expanded cascade failure. After adding the permanent
threshold and medium tests and strengthening archive contracts, the focused
compression set passed 5/5 tests in 28.81 seconds. The four stored-transfer
differences in the separate rotated-medium diagnostic remain unresolved.
