# Configurable collision energy and hard pT range

Both native runners and the Pythia8 input probe accept:

```
--ecm GEV --pt-min GEV --pt-max GEV
```

Defaults remain sqrt(s)=5020 GeV and 20 < pThat < 100 GeV. Values must be
finite and positive; the adapter requires PTMAX > PTMIN and 2*PTMIN < sqrt(s).
Repeated flags, missing values and trailing nonnumeric text are rejected.
These are hard-process pT cuts, not cuts on the final reconstructed jets.
The current interface requires a finite positive upper cut; JEWEL's negative
PTMAX sentinel is not exposed by these options.

The common parser applies them to `PythiaInputConfig`. Pythia settings in the
run header record their exact effective values. With `--table-energy jewel`,
the existing source EOVEST prescription receives the new PTMAX and sqrt(s):

```
min(1.5 * (PTMAX + 50) * cosh(ETAMAX), sqrt(s)/2)
```

The runner's ETAMAX remains 2. This change does not alter medium rapidity,
final-jet analysis acceptance, ISR/MPI policy, PDFs or hadronization settings.
Explicit `--table-energy GEV` and the legacy 100 GeV default remain independent
of the hard cuts. All four initialized table families and replay fixtures use
the selected table energy.

## Validation

The recorded test generates 100 hard events per changed configuration and
checks every pThat against its requested interval. It also runs a native vacuum
event and verifies both the run header and all replay table configurations:

| sqrt(s), GeV | pThat range, GeV | JEWEL table energy, GeV | Native outcome |
|---|---|---|---|
| 2760 | 50–400 | 1380 (beam-energy cap) | event |
| 13000 | 40–60 | 620.7622890287992 | event |

Default and explicitly specified default inputs reproduce identical records,
including complete native-runner output. Forty-eight malformed/range-invalid
inputs are rejected before creating the output file. The input, ISR CLI and
shower-output regression checks are run alongside the new CLI test.
[Executable hashes and results](validation-hard-range.json).

These checks establish configuration propagation and unchanged defaults. They
do not establish matched Fortran/Pythia8 physics or the requested jet-ratio gate.
The original Fortran EOVEST expression and the existing table differential
checks are documented in [table-energy validation](validation-table-energy.md).

```sh
build-pythia8/jewel-run-events events.jsonl 1000 2601001 vacuum \
  --ecm 5020 --pt-min 50 --pt-max 400 --table-energy jewel
ctest --test-dir build-pythia8 -R '^hard-range-cli$' --output-on-failure
```
