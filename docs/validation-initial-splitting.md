# Initial-state splitting validation

`make_initial_splitting` translates the complete MAKEINSPLIT routine from
hash-verified JEWEL 2.6.0. The reference calls the unchanged original body,
GETMASS and the existing Pythia6 kinematic helpers. Production code remains
C++ and uses the owned shower records and explicit random source.

Of 840 cases, 781 have wholly finite reference outputs and pass all 51,044
scalar comparisons. Maximum absolute difference is 5.33e-15, with relative
acceptance 2e-12 and absolute 2e-13. Status, flavour, primary/daughter IDs,
colour labels, pair/ordering flags, discard flag and random counts match
exactly. Maximum random consumption is 80 draws.

The finite cases include 501 accepted splits and 280 kinematic rejections.
Of the accepted splits, 429 leave a spacelike continuing parton and 207 have
a massive radiated daughter. The matrix covers GG, QG, GQ and QQ transitions,
quarks and antiquarks, ordinary/recoil parent statuses, NF=3/5, angular modes
0/5, three energy fractions, integer/noninteger parent masses and virtualities,
two transfer scales, two formation factors and rotated input momenta.

Checks include the complete parent and daughter records: momenta, signed mass,
auxiliary vertices, production/decay times, branch metadata, genealogy and
colours. Independent checks verify four-momentum conservation, colour-label
balance and identical production points for the two daughters. C++ contracts
also check capacity rejection before RNG consumption, a kinematic rejection
that returns X=1, signed outgoing virtuality, ancestry and invalid parameters.

## Legacy fallback and precision behavior

The source first samples the radiated mass. If the transverse momentum squared
is negative, it resets that mass to zero and tries again. If still impossible,
it restores the parent's frame and returns X=1 without adding daughters.
The port preserves these decisions and the metadata retained during fallback.

For a recoil parent, the mass-reset branch sets ZDEC=-1. If the zero-mass retry
succeeds, the source later evaluates an angle through sqrt(ZDEC*(1-ZDEC)),
producing NaN. This occurs in 59 fixtures. C++ rejects all 59 with a domain
exception; draw counts before rejection agree with the source. These cases are
listed individually and excluded from the finite-parity count. The eventual
event driver must restore or discard such candidates.

At the collinear boundary, the transverse momentum calculation involves
cancellation. Explicit fused subtraction reproduces the reference's evaluation
and removes amplified square-root differences without adjusting tolerances or
changing the Fortran body.

## API and scope

`InitialSplitRequest` supplies energy fraction, summed transfer, virtuality,
incoming flavour, current time, radiation time, formation factor and mass scale.
The return value carries the updated fraction and stable IDs of the radiated
and continuing daughters; zero IDs indicate that no daughters were added.
The energy fraction must be strictly between zero and one for a split request.
The sampler and request must share the run's scales/settings.

The capacity flag, state mutations and consumed random draws follow the source
on the tested domain. Exceptions may leave partially modified state and require
caller rollback/discard. A recoil parent's completed split changes its status
to 13; other parents become status 11. Daughter production positions use the
restored parent momentum and max(|p|,E), as in the original.

All six affected shower/reference tests pass, and the new contract test passes
in the Fortran-free C++ build. This establishes the split-construction routine,
not the initial-state scattering scheduler, coherent evolution or full events.

[Detailed results and provenance](validation-initial-splitting.json).
