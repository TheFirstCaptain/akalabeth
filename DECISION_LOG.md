# Decision Log

Record durable architecture, tooling, workflow, compatibility, and behavior decisions here. Keep entries factual and dated.

## 2026-06-28: Establish C Harness Before Modern App Shell

- Decision: Start modernization with a portable C `Core/` plus command-line `harness/` tests before adding a macOS app shell.
- Rationale: The repository currently contains an Applesoft BASIC listing and historical assets, not an existing modern project. A small harness lets future work characterize source structure and extracted rules without committing too early to UI technology.
- Consequences: Early tasks should add focused tests and extracted portable behavior. A Swift/AppKit shell can be introduced later after enough core contracts exist.

## 2026-07-12: Use SwiftPM And AppKit For The Native Mac Shell

- Decision: Add a Swift Package with a `CAkalabeth` C target, an `AkalabethMac` Swift session library, and an `AkalabethApp` AppKit executable.
- Rationale: SwiftPM builds the existing portable C core directly, keeps command/input orchestration testable without a window, and provides a small native shell suitable for Apple silicon before packaging work.
- Consequences: F-010 produces an arm64 AppKit executable and deterministic fixture/smoke launch routes. Signed `.app` bundle metadata, persistence, settings, and distribution packaging remain F-011 work.
