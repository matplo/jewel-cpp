# Common Fortran/native event analysis

The Fortran HepMC2 output now enters the same hepyy/cppyy FastJet analysis as
native C++ JSON output, including the fixed jet mass and Soft Drop definitions.
The importer uses hepyy's native HepMC3 3.3.1 ReaderAsciiHepMC2 through cppyy;
it does not require a separate pyhepmc installation.

## Import contract and normalization

`tools/import_reference_hepmc.py` reads status-1 particles, preserving PDG IDs,
four-momenta, event numbers and the single event weight. Beams, status-23 hard
partons and scattering-centre diagnostics are excluded. Neutrinos remain in
the interchange data and are removed by the common jet analysis. No additional
thermal subtraction or on-shell projection is applied: these particles are
exactly the final-state output of the reference run.

The interchange schema is `jewel-fortran-hepmc-analysis-v1`, explicitly distinct
from native generator output. It records source/card/importer hashes, complete
main and medium cards, known output-policy settings, per-event cross sections
in pb, and the final running cross-section estimate. Omitted card settings are
recorded as unknown rather than silently inferred. The supplied mode labels
the run; it is not inferred from the filename or particle content.

Import requires positive NEVENT, explicit KEEPRECOILS T/F and WRITEDUMMIES F.
It validates a complete HepMC2 end marker, GEV units, finite momenta and a
single finite weight, unique event IDs and exact agreement with NEVENT.
Unsupported dummy-particle removal is rejected. Output is written atomically
only after these checks; existing output is refused.

**Written reference events are not generated attempts.** Their denominator is
`written-events`, and internal generation/cascade retry counts are unknown,
represented by null failure counters. Native denominators remain
`attempted-events`, with failed attempts contributing no jets. The legacy
interchange keys `attempt` and histogram `jets_per_attempt` mean ordinal input
record and yield per denominator record for this reference schema; the explicit
`denominator_scope` is authoritative. The report labels this distinction and
never reports unknown reference failures as zero. Cross sections are preserved
but not applied to these histograms. A mixed input set cannot be assigned one
`--generator` executable hash. The artifact has no common executable hash:
historical native generator provenance is in native-substructure-ensemble.json,
and this reference smoke check does not establish a frozen binary provenance.

## Evidence

The existing custom Fortran vacuum smoke file contains three events and 506
status-1 particles. An independent text-parser test compares every PDG ID and
four-vector exactly against the native HepMC3 import. All three weights and
event IDs also agree. Truncation, incorrect units/event counts, duplicate cards,
duplicate events, unsupported dummy output and overwrite tests pass, with no
published partial outputs after errors.

Common analysis selects eight jets from this file. The historical native
vacuum/medium inputs retain 1081/1295 selected jets, with all six distributions
and all observable accounting unchanged. Every histogram count and weighted
in-range sum closes for all samples. See the
[common event browser](common-event-report.html) and
[machine-readable evidence](common-event-analysis.json). Report JavaScript
passes a syntax check; the report has not been visually reviewed.

These are **not equivalent MC samples**. The smoke reference has hard cuts
50–400 GeV and ETAMAX=3.1, versus the historical native 20–100 GeV setup.
Stock INITPYTHIA explicitly sets MSTP(61)=1 (ISR on); the current native runs
have ISR off. PDFs, fragmentation, all remaining run defaults and cross-section
normalization also require a matched configuration audit. There is no ratio
acceptance claim, and eight reference jets do not satisfy the >=1000-jet gate.

## Reproduction

Use the environment with hepyy/cppyy, HepMC3, FastJet and hepyy-utils:

```sh
python tools/import_reference_hepmc.py events.hepmc --cards params.dat \
  --medium-cards medium.dat --mode vacuum --output reference.jsonl
python tools/analyze_native_events.py native.jsonl reference.jsonl \
  --output analysis.json
python tools/render_native_report.py analysis.json report.html
python tests/reference_hepmc.py events.hepmc params.dat medium.dat
```

The corruption test requires at least two uniquely numbered events so it can
exercise duplicate-event rejection.
