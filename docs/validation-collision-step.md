# Combined collision-step validation

The `scatter_pair` and `finish_collision` operations now cover a complete
transfer step through DOKINEMATICS line 6385: momentum reconstruction, outgoing
statuses, colour connections, branching metadata, production vertices, decay
times and recoil/subtraction storage. The hash-verified reference contains the
unchanged statements from lines 5752–6385 and original declarations. The wrapper
supplies a single loop index; it does not execute the enclosing transfer loop,
final decay-time overwrite or outer target-mass retries.

The matrix contains 768 cases. All 750 finite reference cases pass 100,500
scalar checks. Maximum raw absolute difference is 3.56e-15. Status, flavour,
colour labels, pair flags, stored counts and random-draw counts match exactly.
There are 366 accepted transfers and 384 rejected/zero transfers; 31 cases
sample a splitting recoil. Outgoing recoil statuses 1, 2, 3 and 13 are covered.
Maximum random consumption is 23 draws.

Coverage combines KINMODE 0–3, RECMODE 0–3, quarks, antiquarks and gluons,
hadronized/omitted recoils, soft cuts, hard-recoil rescattering, zero/nonzero
splitting fractions, coherent/ordinary decay-time choices, first/intermediate
storage occupancy and last/intermediate transfer timing. All allocated storage
slots, including untouched sentinels, are compared. Independent checks verify
net colour-label balance, identical production coordinates for the two outgoing
records, transfer time on the incoming record and unchanged storage/colour
allocation for rejected transfers.

## Precision and nonfinite cases

Ordinary numerical fields use relative 2e-12 / absolute 2e-13. Dummy components
use absolute 2e-20. Five nearly lightlike dummy masses amplify sub-ulp momentum
differences through the square root. Their raw masses do not satisfy that tiny
absolute bound, so they are retained individually in the report and compared
as signed mass squared with a propagated roundoff bound:

`16 * epsilon * sum(p_ref²) + sum(2 * abs(p_ref) * abs(delta_p) + delta_p²)`.

The sum runs over the four dummy-momentum components. This accounts for the
short multiply/subtract calculation and the measured differences in its inputs;
it does not assert raw mass equality. The largest signed-invariant difference
in these cases is 3.54e-28 GeV². All five lie within the bound. Signs, ordinary
momenta and the other fields retain their separate checks.

Eighteen reference cases produce nonfinite subtraction dummies: in KINMODE=0,
RECMODE=2 (or soft RECMODE=3), the stored momentum difference can be timelike,
while the source's dummy formula assumes a spacelike difference. C++ rejects
these domain failures instead of writing NaNs. The individual inputs and
rejections remain in the report, outside the finite-parity count. Random
consumption before rejection agrees. No finite agreement is claimed for them.

## Integration boundary

`finish_collision` mutates candidate records, colour allocation and the recoil
ledger. It should run once per accepted transfer. Exceptions can leave those
candidate objects partially modified: the enclosing sequence must restore or
discard them. Input momenta remain value-owned by `scatter_pair`.

Event IDs and parent/daughter links are not assigned by these operations. The
source assigns no primary parent to a recoil even though its incoming projectile
lists it among the outgoing records; that distinction must be represented when
integrating the event graph. The original sequence projectile must also be
retained for temperature-dependent recoil masses across multiple transfers.
The source's final-time overwrite can draw a second lifetime after the last
step and must not be omitted from the forthcoming sequence implementation.

All six directly affected tests (kinematics, recoil, collision and combined-step
comparisons/contracts) pass. C++ contracts independently check colour allocation,
production/decay times, ledger insertion, invalid fractions and formation factors.

[Detailed results and provenance](validation-collision-step.json).
