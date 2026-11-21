# Legacy MPI saved-state audit

A diagnostic-only patch to the frozen Pythia6 PYMULT routine records its saved
initialization state and the first ten calls each to modes 4, 5 and 6. It adds
writes and counters only; it changes no physics values or random draws.
The complete 1,000-attempt run reproduces the baseline HepMC byte for byte:
`64a5289388223a4fb5e1b272007a431c773b5125cb3277e66d6718733e275ca3`.

## Measured state

| Quantity | First pp PYMULT initialization | Sampled production calls |
|---|---:|---:|
| PARP(82), GeV | 2.0 | 1.8 |
| Effective pT0 at 5020 GeV, GeV | 2.5845670224 | 2.3261103202 |
| VINT(149), dimensionless screening squared | 1.0602985180023890e-6 | 8.5884179958193529e-7 |
| Saved VNT145, mb | 124.73191966455620 | 124.73191966455620 |
| Saved VNT146 | 0.83059932866738195 | 0.83059932866738195 |
| Saved VNT147 | 0.091240840189946285 | 0.091240840189946285 |

All 20 saved SIGM bins are identical to the first initialization at all 30
sampled production entries. The screening squared decreases by a factor of
0.81. This confirms the combination of old saved normalization/sampling bins
and a new shared screening scale; it is not merely a post-PYINIT logging artifact.
The trace is bounded: no claim is made that every production call was inspected.

PYINIT invokes PYMULT(1), then PYMIGN(1). The second initializer lowers the shared
PARP(82) and VINT(149), but cannot overwrite PYMULT's local SAVE variables.
PYMULT restores VINT(145:147) from those local values on entry to its production
branches. Thus the 58.7021 mb value observed immediately after PYINIT belongs to
the secondary initializer, not the legacy production normalization.

In mode 4, the current VINT(149) sets the bin coordinate. In mode 5, the saved
SIGM/NMUL table enters SIGABV while current VINT(149) enters its conversion;
VNT146 and VNT147 enter the impact-parameter weighting. Mode 6 restores the same
saved normalization before generating additional interactions. The later three
initializations are separate zero-event isospin channels and must not replace
the first pp initialization when interpreting this sample.

## Port consequence

Matching a Pythia8 pT0 value, coupling and double-Gaussian shape does not reproduce
this two-stage legacy state. The earlier shifted-PDF diagnostic's approximately
149 mb integral is evaluated at the reduced screening scale; it must not be
compared as though the source used a newly integrated legacy table at that scale.
No arbitrary integral rescaling or retuning is introduced here.

The next implementation decision needs an explicit legacy MPI initialization
policy: separately represent the table/overlap initialization scale and the live
production screening scale, then validate the resulting normalization and
sampling against PYMULT. Pythia8's current single-state MPI initialization is
not yet a demonstrated replacement. ISR ordering, channel/flavour selection and
other cross-section differences also remain unresolved. Full jet/substructure
parity is still not established.

## Reproduction and evidence

Apply [the instrumentation patch](../reference/mpi_saved_state.patch) to a copy
of the frozen `pythia6425mod-lhapdf6.f`. Compile with the same `-O2 -std=legacy
-ffixed-line-length-none` flags and link the existing reference's JEWEL core,
medium-vac, meix, subtraction and LHAPDF objects/libraries, replacing only the
Pythia6 object. Run the unchanged reference cards in a fresh directory, recording
stdout as `run.log`. Preserve the patched source as `pythia.f`, executable as
`reference` and applied patch as `instrumentation.patch` for the analyzer.

```sh
python3 tools/analyze_mpi_saved_state.py /path/to/diagnostic-run \
  /path/to/baseline/events.hepmc /path/to/original/pythia6425mod-lhapdf6.f \
  saved-state.json
```

The [saved report](validation-mpi-saved-state.json) contains all sampled bins and
normalizations, original-source hash, patched-source and executable hashes,
card hashes, log hash and event-output identity. No installed reference source
or native production physics was modified by this audit.
