# Lantern tests

Run `build.cmd` once from the repository root, then invoke:

```bat
tests\test-equipment.cmd
tests\test-localization.cmd
tests\test-content.cmd
```

Launchers resolve the project from their own location and work from any current
directory. Binaries remain under `build/`. Content tests require the sibling
CarryOn checkout and private native fixtures described in the main README.

`test-lantern-model.py` checks independent model decoding (both the default and
`--gold` variants); see the main README for decoder dependencies.
`repair_inventory.py` checks the offline repair against two explicitly supplied
private character fixtures. No character saves are included in this repository.
The reusable SDK keeps its own tests under `sdk/tests/`.
