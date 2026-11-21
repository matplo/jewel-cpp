# Recoil storage validation

The C++ `store_recoil` helper translates the recoil-storage section of
DOKINEMATICS, JEWEL 2.6.0, lines 6261–6376. This is a section-level comparison,
not validation of the complete collision routine. The reference extractor
checks both the original full-source SHA-256 and the section SHA-256, then
places the unchanged statements in a declaration-only test wrapper. No C++
results are injected into the reference calculation.

All 1,664 cases (49,536 compared scalars) pass. The largest absolute
difference is 1.58e-20. Discrete values match exactly; numerical acceptance
uses relative 2e-12, absolute 2e-13 for centres and 2e-20 for dummy values.
There are 213 negative-energy transfer cases and 338 untouched soft slots.

The replay matrix covers all four RECMODE values, soft/hard recoil handling,
write/subtraction enable flags, rejected transfers, empty/full capacity, and
positive/negative transfer energy. Inputs include 160 randomized on-shell
incoming/outgoing pairs, each in eight mode/soft combinations. Every allocated
slot is initialized to a nonzero sentinel and compared, including inactive
storage, so skipped writes cannot silently become zeroed records.

Independent checks verify stored counts, flavour, fixed dummy energy,
projection direction (including negative-energy sign handling), mass sign,
and preserved soft slots. C++ contracts additionally exercise rollback by
copy/restore, first allocation, invalid mode/bounds and zero-energy projection.

The source advances NSCATCEN for soft recoils in modes 0/1 without filling the
slot. The port preserves this: reused slots retain prior contents, and newly
allocated slots are value-initialized. Downstream export must not infer that
all counted slots necessarily contain a newly written centre. This is an
explicit legacy behavior, not a proposed change to the physics model.

`RecoilStorage` owns the full allocated tail separately from its used count.
Rollback must preserve the intended tail contents as well as the count.
For undefined dummy projections (zero energy or nonphysical square root),
C++ throws instead of accepting Fortran NaN/Inf; no equivalence is claimed for
those inputs. An exception can leave storage partially modified, so the caller
must discard or restore the enclosing candidate operation.

The helper does not decide whether a recoil is soft, perform subtraction on
final particles, or apply collision kinematics/colour changes. Those remain
caller responsibilities in the forthcoming complete DOKINEMATICS port.

[Results, input hash and source/executable hashes](validation-recoil.json).
