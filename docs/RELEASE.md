# Akalabeth Release Checklist

This checklist is for local distribution builds and operator-owned signing/notarization. The repository does not store signing identities, Apple credentials, or notarization profiles.

## Build Inputs

- macOS 13 or newer.
- Xcode command line tools with Swift 6 support.
- Apple silicon host for the primary release artifact.
- Optional signing identity in the local keychain.
- Optional notarization profile created with `xcrun notarytool store-credentials`.

## Clean Build

From a clean checkout:

```bash
swift build -c release
swift test
make -C harness test
Scripts/package_macos_app.sh --require-clean
```

The package script builds `.build/release/Akalabeth.app`, lints `Info.plist`, verifies the executable is an arm64 Mach-O binary, and runs the packaged app smoke test:

```bash
.build/release/Akalabeth.app/Contents/MacOS/Akalabeth --smoke-test
```

For a local work-in-progress build where the checkout is intentionally dirty, omit `--require-clean`.

## Manual Validation

Use a clean app data directory before release validation:

```bash
rm -f "$HOME/Library/Application Support/Akalabeth/SaveGame.json"
defaults delete dev.local.akalabeth 2>/dev/null || true
```

Then validate:

- Launch `.build/release/Akalabeth.app`.
- Start a normal game through lucky number, level, character accept/reroll, and class selection.
- Change display color, scale, high contrast, scanlines, and audio feedback; quit and relaunch to confirm settings persist.
- Use Game > Save Game, quit, relaunch, and confirm the save resumes.
- Use Game > Delete Saved Game, quit, relaunch, and confirm the app starts a new session.
- Use Game > New Game and confirm the save is cleared.
- Launch deterministic fixtures from the Game menu: town, overworld, dungeon, and quest.
- Confirm Help > Akalabeth Help and About Akalabeth open and describe modern save/resume behavior as an app-layer convenience.

## Signing

Unsigned local packages are suitable for development only. To sign during packaging, provide a local Developer ID Application identity:

```bash
AKALABETH_BUNDLE_ID="com.example.Akalabeth" \
AKALABETH_VERSION="0.1.0" \
AKALABETH_BUILD_NUMBER="1" \
AKALABETH_CODESIGN_IDENTITY="Developer ID Application: Example Name (TEAMID)" \
Scripts/package_macos_app.sh --require-clean
```

The script uses hardened runtime options and verifies the signed bundle with `codesign`.

## Notarization

Create a zip from the signed app:

```bash
ditto -c -k --keepParent .build/release/Akalabeth.app .build/release/Akalabeth.zip
```

Submit with an operator-owned notarytool profile:

```bash
xcrun notarytool submit .build/release/Akalabeth.zip \
  --keychain-profile "AkalabethNotary" \
  --wait
```

Staple and verify:

```bash
xcrun stapler staple .build/release/Akalabeth.app
spctl --assess --type execute --verbose=4 .build/release/Akalabeth.app
codesign --verify --strict --deep --verbose=2 .build/release/Akalabeth.app
```

## Release Notes

Record the following for each release:

- Git commit.
- `AKALABETH_VERSION` and `AKALABETH_BUILD_NUMBER`.
- `swift test` result.
- `make -C harness test` result.
- Package script result.
- Smoke-test result.
- Signing identity used, if any.
- Notarization submission ID, if any.
