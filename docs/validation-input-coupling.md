# Explicit hard and ISR coupling controls

The hard-input, parton and hadron runners share these options:

```sh
--hard-alpha-s 0.11754434294459773 --hard-alpha-s-order 1 \
--isr-alpha-s 0.11754434294459773 --isr-alpha-s-order 1 \
--alpha-s-nfmax 5 --isr-alpha-s-cmw off
```

These values translate the measured coupling for the existing Fortran reference
cards, as derived in [the effective coupling audit](validation-effective-couplings.md).
They are not a universal JEWEL tune. Omitting the options preserves the previous
settings. The maximum flavour count is Pythia's **global** StandardModel setting,
so it also affects other Pythia coupling instances, including MPI. It is distinct
from JEWEL's NF setting used by its own kernels. CMW controls only SpaceShower.

Each value/order pair must be supplied together. Orders 0–3, couplings 0.06–0.25,
flavour caps 5/6 and CMW on/off follow the installed Pythia8 settings ranges.
Explicit settings are applied after Tune 14 and checked again after initialization;
a changed setting is an error. Run headers record the requested overrides.

The integration test generates three ISR/MPI hard-input events and three complete
vacuum events, verifies the headers, and exercises the alternate CMW-on/nfmax-6
policy. All 28 invalid-input cases fail before creating output. Coupling CLI,
hard-range CLI and Pythia input contracts all pass. Detailed evidence is in
[validation-input-coupling.json](validation-input-coupling.json).

```sh
ctest --test-dir build-pythia8 --output-on-failure \
  -R '^(input-coupling-cli|hard-range-cli|pythia8-input-contracts)$'
```

This verifies integration and preservation of the requested parameters. Coupling
function agreement at common scales does not imply equivalence of ISR evolution
variables, scale selection, MPI or full event generation.
