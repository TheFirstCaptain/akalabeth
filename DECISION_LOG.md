# Decision Log

Record durable architecture, tooling, workflow, compatibility, and behavior decisions here. Keep entries factual and dated.

## 2026-06-28: Establish C Harness Before Modern App Shell

- Decision: Start modernization with a portable C `Core/` plus command-line `harness/` tests before adding a macOS app shell.
- Rationale: The repository currently contains an Applesoft BASIC listing and historical assets, not an existing modern project. A small harness lets future work characterize source structure and extracted rules without committing too early to UI technology.
- Consequences: Early tasks should add focused tests and extracted portable behavior. A Swift/AppKit shell can be introduced later after enough core contracts exist.

