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

The importer creates two inert beam records in physical slots 1 and 2, with
slot 1 also reserved for the cascade scratch record. Each subsystem has two
incoming records parented to those beams, and two distinct original hard
records with no parents. Hard final copies retain ancestry to their respective original
hard records; MPI hard legs separately expose scheduling parent zero; ISR emissions point to their respective incoming records;
remnants follow reciprocal source ancestry back to a beam. Unsupported remnant
ancestry is rejected. These records have status 21 and no fabricated daughters.

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

This is a raw import stage, **not an evolution-ready event**. Parent categories
now follow the stock record distinction, but the compact record is not a
one-to-one copy of Pythia6 history or its full ordering. No parton is activated
and no virtualities are initialized. The native runner's ISR/MPI guard remains
in place. Survivor selection is now connected and checked separately in
[native preparation](validation-native-preparation.md). Source string ordering,
hard/MPI virtuality initialization and cascade integration remain pending.

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

Generated-event contracts verify distinct hard-parent links, primary/MPI
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

## Parent categories and colour angles

The stock 1,000-event trace after `interpretpythiaevent` distinguishes beam
parents (slots 1/2), incoming parents (slots 3/4), and separate original hard
parents (slots 7/8). `GETTHETAM` suppresses angles for colour partners whose
parent slot is at most 2. A shared hard anchor or unresolved ISR/remnant parent
would lose this distinction. The compact native layout preserves the beam
threshold and categories; it omits the stock intermediate slots 5/6.

The new two-seed audit checks 27,490 hard-parent links, 26,932 ISR-parent links,
and beam-parent counts for all 9,670 remnants in the 1,997 mapped events.
The same three frame failures remain recorded. All 134,660 emitted timing
values still match Fortran exactly, and the timing fixtures are byte-identical
to the preceding import run. A focused colour-angle contract checks beam slots
1/2 return the sentinel while incoming/original slots give the geometric angle.

[Parentage audit evidence](validation-native-parentage.json) records both runs.
These are input-record and timing checks, not full-cascade validation.

## MPI scheduling correction

The original parent-category audit used a stock sample with MPI disabled.
An MPI-enabled stock trace shows that the original-hard-parent rule applies
to the primary pair; stock MPI candidates instead have parent zero. Native
MPI legs now retain their original-record ancestry while explicitly setting
source scheduling slots to `{0,0,0}`. GETTHETAM and coherence use those scheduling
links. This corrects the earlier extension of primary parent semantics to MPI.
See [MPI scheduling evidence](validation-mpi-scheduling.md).

Imported fractions explicitly start at zero, matching GENEVENT's `ZD(I)=0`.
The generic ShowerParticle default of -1 is a different sentinel and must not
leak into initial ISR/remnant records. See the [regression and cascade evidence](validation-imported-fractions.md).
