# Actual JEWEL 2.6.0 ISR energy and time records

A 1,000-event stock vacuum run confirms that negative-energy incoming lines
occur in the Pythia6 histories consumed by `interpretpythiaevent`. Every one
of the 116 negative-energy incoming records in this sample has a reversed
production/end-time interval. Signed energies are therefore not, by themselves,
evidence that the proposed Pythia8 history mapping is invalid. This does not
prove that the two generators' histories or virtuality conventions correspond.

| Record class | Lines | Negative energy | Reversed intervals | Nonfinite time/position |
|---|---:|---:|---:|---:|
| Hard incoming | 2,000 | 104 | 104 | 135 |
| Nonterminal incoming ancestors | 3,051 | 12 | 12 | 0 |
| Terminal beam ancestors | 1,865 | 0 | 0 | 0 |
| Emitted siblings | 4,916 | 0 | 15 | 0 |

The minimum hard incoming energy is -237.020864611214 GeV; the minimum
nonterminal ancestor energy is -31.0726449150205 GeV. For all 4,916 finite,
nonterminal incoming timing records, interval orientation agrees with the
energy sign. Emitted siblings use a different rule: exact on-shell mass equality
sets their end time to the medium lifetime, so their interval orientation is not
an incoming-energy sign test. The 15 reversed emitted intervals are retained.

## Zero-virtuality direct-beam lines

All 135 nonfinite records are hard incoming lines whose parent is beam record
3 or 4 and whose recorded mass is zero. The source calculates a zero formation
rate, then obtains an infinite negative production time and infinite production
coordinates. These are actual intermediate source values, not an audit failure
or invented replacement. The JSON records every affected row, with textual
Infinity values preserved from the trace. No nonfinite kinematic input was found.

The C++ `assign_isr_times` currently rejects zero virtuality before consuming a
random draw. Thus its defined-domain differential parity is not a claim of full
behavioral equivalence for direct-beam lines. The adapter still needs an explicit
representation and treatment of these lines, including their downstream use and
RNG consumption. No cutoff, absolute-energy replacement or finite-time clamp was
introduced here.

## Instrumentation and validation

`tools/build_jewel_isr_trace.py` builds an isolated baseline and traced executable
from the frozen stock source. The baseline only removes the known duplicate
`WEIGHT` declaration required by GNU Fortran. The trace adds writes immediately
after each incoming/sibling time calculation, before subsequent record reordering.
It reads existing PYJETS and TIME fields and adds no random draws or Pythia calls.
It records the interpretation attempt, role, side, record/parent indices, PDG,
momentum, recorded mass, production coordinates/time and end time.

`tools/run_jewel_isr_trace.py` runs both binaries with identical cards and tables.
For this sample, **the complete HepMC event file and final `out.ran` state are
byte-identical**. Both contain 1,000 accepted events; the trace contains 1,000
interpretation attempts and 11,832 rows. The two signed timing CTest suites also
pass, including the 1,152-case comparison against the extracted Fortran block.

The sample uses NJOB=901, sqrt(s)=5020 GeV, PTMIN=50 GeV, PTMAX=400 GeV,
ETAMAX=3.1, PDFSET=13100, unweighted PP vacuum events and the frozen reference
tables. Full generated cards, source/build hashes, table hashes, equality hashes,
counts and exceptional rows are in [the evidence](validation-stock-isr-times.json).
It is a source-behavior diagnostic, not a matched Pythia8 physics sample or a
jet-spectrum validation.

```sh
python tools/build_jewel_isr_trace.py "$JEWEL_SOURCE" /tmp/jewel-isr-build \
  --lhapdf-prefix "$LHAPDF_PREFIX"
python tools/run_jewel_isr_trace.py /tmp/jewel-isr-build "$VACUUM_CARD" \
  /tmp/jewel-isr-sample --events 1000 --lhapdf-data "$LHAPDF_DATA_PATH"
ctest --test-dir build -R '^isr-times-(contracts|differential)$' --output-on-failure
```

Both output directories must be new. The card's table and medium paths resolve
relative to its original directory. The builder uses the supplied frozen stock
vacuum medium, not an installed custom executable. The installed JEWEL and
Pythia sources are unchanged.
