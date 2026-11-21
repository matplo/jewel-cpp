# Correct imported event-start splitting fractions

Stock GENEVENT clears `ZD(I)=0.d0` for every record before PYEVNT. The native
importer instead inherited `ShowerParticle::fraction{-1}` for ISR, remnants and
anchors; only hard-pair initialization replaced it. That sentinel is valid in
other contexts but is incorrect for the initial event record.

The importer now explicitly sets fraction zero for all initial records.
No global particle default, collision formula, numerical tolerance or random
stream was changed. Imported angles and other metadata are unchanged.

## Failure localization

The old medium attempt 2 has identical native/Fortran branch-entry traces
through all five visited branches, with zero numerical or discrete differences.
At branch 5, slot 29 carries fraction -1 and zero mass. Both implementations
reach draw 6,052. The native state validator rejects a nonfinite record; Fortran
writes the outgoing particle at slot 55 with fraction -1, mass zero and NaN
angle. Collision finalization evaluates the angle using `sqrt(z*(1-z))`.

Thus reproducing NaN in Fortran did not prove a stock-generated event would
fail: the replay had supplied the same incorrectly initialized fraction to
both engines. The source event-start initialization identifies the correction.

## Same-seed results after the fix

The existing 20-attempt ISR+MPI samples were rerun with identical settings.
Vacuum remains 20/20 complete, with identical parsed output records. Medium
completion rises from 2/20 to **16/20**. Remaining failed attempts are 11, 13
and 18 (nonfinite kinematics) and 19 (nonfinite shower record). Accepted/failed
weights are 16/4 and generator weight sum is 20.

Corrected attempt 2 finishes in both native and Fortran replay with identical
execution headers and draw count 140,129. It has 14 scalar mismatches among
8,505 values at the unchanged strict tolerances, so it is not a numerical
parity pass. Control attempt 14 still passes all 7,045 values with draw count
1,077,377. No unresolved failure or mismatch is counted as agreement.

The imported-record contract checks fraction zero on every record, including
ISR and remnant records. The subsystem-initialization contract passes. The
experimental runner contract additionally exercises medium attempts 1 and 2,
requiring both to complete, preventing recurrence of this specific NaN.

[Machine-readable evidence](validation-imported-fractions.json) contains the
branch trace comparison, before/after summaries, remaining failures, hashes
and corrected Fortran replay outcomes. Full-generator jet validation remains
unfinished.
