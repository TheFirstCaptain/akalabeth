# Validation Strategy

## Automated Validation

Run the current harness:

```bash
make -C harness test
```

Add a focused test whenever behavior is extracted from the BASIC listing.

## Source Characterization

- Verify important line numbers exist before extracting behavior.
- Verify line ranges contain expected commands, labels, and data tables.
- Cross-check important `AKLABETH.TXT` line numbers and contents before changing source-preservation behavior.

## Behavior Characterization

- Prefer deterministic fixtures over broad rewrites.
- Keep test names tied to source line ranges when practical.
- Record inferred behavior and unresolved typos or listing ambiguities in docs or feature notes.

## Manual Validation

For rendering, input, and gameplay flows, include:

- Exact command to launch the app or harness.
- Initial state or fixture.
- User actions.
- Expected observations.
- Known residual risk.
