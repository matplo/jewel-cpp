# Source-derived diquark Lund mapping

The JEWEL mapped fragmentation profile now sets aExtraDiquark=0.5,
aExtraSQuark=0, and useOldAExtra=off explicitly. This corrects a remaining
Pythia8 Tune-14 inheritance: the effective values before mapping were 0.97
and useOldAExtra=on, verified through the installed Pythia8 settings interface.
These values were selected from source semantics, not fitted to jet ratios.
The ordinary Monash profile remains unchanged.

The frozen Pythia6 block data initialize PARJ(45)=0.5. JEWEL INITPYTHIA does not
override that parameter. PYZDIS sets FA=PARJ(41)+PARJ(45) for a new diquark,
and FC=1-PARJ(45)*old_diquark+PARJ(45)*new_diquark. In Pythia8 8.317, the corrected
StringZ::initShape uses precisely these old/new assignments when useOldAExtra
is off. Tune 14 turns the compatibility flag on, despite the XML default being
off; therefore changing only the parameter magnitude would not suffice.

## Validation

The [reference extractor](../reference/extract_lund_shape.py) checks the frozen
Pythia6 source hash and copies the PYZDIS declarations and shape statements
unchanged, renaming the entry point and returning before random sampling.
The [driver](../reference/lund_shape_driver.f90) uses actual block-data defaults
and JEWEL's single-precision-literal assignments for aLund and bLund.

The [native test](../tests/pythia8_lund_shape.cpp) calls the installed, compiled
StringZ::initFlav and initShape methods through a test subclass. It loads settings
after Tune 14 and the JEWEL profile. It compares all combinations of old/new
light quark, strange quark, light diquark and strange diquark at three transverse
mass-squared values. All 48 cases / 144 shape coefficients agree exactly with
the extracted Fortran statements on this platform. The CTest additionally
asserts these frozen-source coefficient values so it fails on the old orientation
or parameter magnitude. [Values, hashes and compiler command](validation-diquark-lund.json).

The fragmentation, neutral-pion runner, native ISR/MPI and Lund-shape tests all
pass. The strengthened shape assertion was rebuilt and rerun successfully.

This validates light/strange longitudinal shape coefficients only, not the
complete fragmentation distributions or flavour selection. Heavy flavour masses,
popcorn/baryon treatment, transverse momentum and termination settings, particle
data and hard/ISR/MPI tune differences still need audit. The prior interactive
jet reports used the old mapped profile; their numerical results are historical
and do not establish the corrected profile's jet ratios. No acceptance threshold
or analysis bin was changed.
