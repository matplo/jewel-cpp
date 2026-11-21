# Event-record compression: boundary routine

`compress_records` in `jewel/compression.hpp` ports stock COMPRESSEVENT at a
transient generator-record boundary. It is not yet wired into Cascade or used
as the persistent shower storage. Existing shower behavior remains unchanged.

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

## Scheduler integration still required

The stable-ID graph enforces reciprocal genealogy; raw source compression
clears genealogy on moved records and leaves the earlier prefix untouched.
Blindly restoring the compressed records would violate graph invariants or
silently alter the genealogy used for coherent evolution. The adapter needs
to separate source scheduling links from retained physical ancestry, map the
protected stable ID, and preserve physical-slot evolution metadata. It must
also keep historical ancestry available for jet association after compression.

MAKEBRANCH calls compression after time initialization when N > 20000; its
selected slot can change. Validation must cover that ordering, the surrounding
MAKECASCADE slot visit order, repeated compression and the 22990-record guards.
The existing reference scheduler still aborts if its disabled compression stub
is unexpectedly reached. No claim of compression-enabled cascade parity is
made by this isolated routine comparison.

Run the checks with:

```sh
ctest --test-dir build -R '^compression-' --output-on-failure
```

Both tests pass. The broader 45-test regression result from the preceding
collision-retry checkpoint remains separate; it was not rerun for this new,
currently isolated boundary routine.
