#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$ROOT_DIR/.build/release"
APP_DIR="$BUILD_DIR/Akalabeth.app"
CONTENTS_DIR="$APP_DIR/Contents"
MACOS_DIR="$CONTENTS_DIR/MacOS"
RESOURCES_DIR="$CONTENTS_DIR/Resources"
EXECUTABLE="$MACOS_DIR/Akalabeth"
BUNDLE_ID="${AKALABETH_BUNDLE_ID:-dev.local.akalabeth}"
VERSION="${AKALABETH_VERSION:-0.1.0}"
BUILD_NUMBER="${AKALABETH_BUILD_NUMBER:-1}"
CODESIGN_IDENTITY="${AKALABETH_CODESIGN_IDENTITY:-}"

require_clean=0
skip_smoke=0

for argument in "$@"; do
  case "$argument" in
    --require-clean)
      require_clean=1
      ;;
    --skip-smoke-test)
      skip_smoke=1
      ;;
    *)
      echo "Unknown argument: $argument" >&2
      echo "Usage: $0 [--require-clean] [--skip-smoke-test]" >&2
      exit 64
      ;;
  esac
done

cd "$ROOT_DIR"

if [[ "$require_clean" -eq 1 ]] && [[ -n "$(git status --porcelain)" ]]; then
  echo "Refusing to package from a dirty checkout because --require-clean was supplied." >&2
  exit 1
fi

swift build -c release

rm -rf "$APP_DIR"
mkdir -p "$MACOS_DIR" "$RESOURCES_DIR"
install -m 755 "$BUILD_DIR/AkalabethApp" "$EXECUTABLE"
install -m 644 "$ROOT_DIR/Packaging/AppIcon.svg" "$RESOURCES_DIR/AppIcon.svg"
printf 'APPL????' > "$CONTENTS_DIR/PkgInfo"

cat > "$CONTENTS_DIR/Info.plist" <<'PLIST'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>CFBundleDevelopmentRegion</key>
  <string>en</string>
  <key>CFBundleDisplayName</key>
  <string>Akalabeth</string>
  <key>CFBundleExecutable</key>
  <string>Akalabeth</string>
  <key>CFBundleIdentifier</key>
  <string>__BUNDLE_ID__</string>
  <key>CFBundleInfoDictionaryVersion</key>
  <string>6.0</string>
  <key>CFBundleName</key>
  <string>Akalabeth</string>
  <key>CFBundlePackageType</key>
  <string>APPL</string>
  <key>CFBundleShortVersionString</key>
  <string>__VERSION__</string>
  <key>CFBundleVersion</key>
  <string>__BUILD_NUMBER__</string>
  <key>LSApplicationCategoryType</key>
  <string>public.app-category.games</string>
  <key>LSMinimumSystemVersion</key>
  <string>13.0</string>
  <key>NSHighResolutionCapable</key>
  <true/>
  <key>AkalabethIconSource</key>
  <string>AppIcon.svg</string>
</dict>
</plist>
PLIST

sed -i '' \
  -e "s/__BUNDLE_ID__/$BUNDLE_ID/g" \
  -e "s/__VERSION__/$VERSION/g" \
  -e "s/__BUILD_NUMBER__/$BUILD_NUMBER/g" \
  "$CONTENTS_DIR/Info.plist"

plutil -lint "$CONTENTS_DIR/Info.plist"
file "$EXECUTABLE" | tee "$BUILD_DIR/Akalabeth.file.txt"

if ! grep -q "Mach-O 64-bit executable arm64" "$BUILD_DIR/Akalabeth.file.txt"; then
  echo "Packaged executable is not an arm64 Mach-O binary." >&2
  exit 1
fi

if [[ "$skip_smoke" -eq 0 ]]; then
  "$EXECUTABLE" --smoke-test
fi

if [[ -n "$CODESIGN_IDENTITY" ]]; then
  codesign --force --options runtime --timestamp --sign "$CODESIGN_IDENTITY" "$APP_DIR"
  codesign --verify --strict --deep --verbose=2 "$APP_DIR"
else
  codesign --verify --deep --strict "$APP_DIR" >/dev/null 2>&1 || true
fi

echo "Packaged $APP_DIR"
