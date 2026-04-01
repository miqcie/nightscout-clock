#!/usr/bin/env bash
# Creates GitHub issues for the nightscout-clock improvement plan.
# Usage: GITHUB_TOKEN=ghp_xxx ./scripts/create_issues.sh
#
# Requires: curl, a GitHub personal access token with repo scope

set -euo pipefail

REPO="miqcie/nightscout-clock"
API="https://api.github.com/repos/${REPO}/issues"

if [[ -z "${GITHUB_TOKEN:-}" ]]; then
  echo "Error: GITHUB_TOKEN environment variable is required."
  echo "Usage: GITHUB_TOKEN=ghp_xxx $0"
  exit 1
fi

create_issue() {
  local title="$1"
  local body="$2"
  local labels="$3"

  echo "Creating issue: ${title}..."
  local response
  response=$(curl -s -w "\n%{http_code}" -X POST "$API" \
    -H "Authorization: token ${GITHUB_TOKEN}" \
    -H "Accept: application/vnd.github+json" \
    -d "$(jq -n \
      --arg title "$title" \
      --arg body "$body" \
      --argjson labels "$labels" \
      '{title: $title, body: $body, labels: $labels}')")

  local http_code
  http_code=$(echo "$response" | tail -1)
  local body_response
  body_response=$(echo "$response" | sed '$d')

  if [[ "$http_code" == "201" ]]; then
    local url
    url=$(echo "$body_response" | jq -r '.html_url')
    echo "  ✓ Created: ${url}"
  else
    echo "  ✗ Failed (HTTP ${http_code}): $(echo "$body_response" | jq -r '.message // "unknown error"')"
  fi
}

# ─── Phase 1: Quick Wins ────────────────────────────────────────────

create_issue \
  "Fix typo: intarval_type → interval_type in BGDisplayManager.h" \
  "$(cat <<'EOF'
## Problem

There is a typo in `src/BGDisplayManager.h` line 24: `intarval_type` should be `interval_type`.

There is also a stale `/// TODO: Add urgent values to settings` comment in `BGDisplayManager.cpp` that should be resolved or removed.

## Effort
~10 minutes

## Labels
Phase 1 — Quick Wins
EOF
)" \
  '["bug", "good first issue"]'

create_issue \
  "Add settings validation on config load" \
  "$(cat <<'EOF'
## Problem

`SettingsManager` loads JSON config from LittleFS but does not validate value ranges. Out-of-range values silently produce odd behavior (e.g. invalid brightness, nonsensical intervals, alarm thresholds).

## Proposed Solution

- Add range-checking in `SettingsManager` during deserialization
- Clamp out-of-range values to their defaults and log a warning via `DEBUG_PRINTF`
- Fields to validate at minimum:
  - `brightness_level` (0–255)
  - `face_rotate_interval_sec` (already has 5–120, but verify enforcement)
  - `alarm_high_threshold`, `alarm_low_threshold` (physiological BG ranges)
  - `alarm_snooze_minutes`
  - Any other numeric settings with meaningful bounds

## Effort
~2 hours

## Labels
Phase 1 — Quick Wins
EOF
)" \
  '["enhancement"]'

create_issue \
  "Make weather fallback location configurable instead of hardcoded Richmond, VA" \
  "$(cat <<'EOF'
## Problem

`BGDisplayFaceWeather.cpp` (lines 18-19) falls back to hardcoded Richmond, VA coordinates when zip geocoding fails. Users outside the US will see incorrect weather data with no indication of why.

## Proposed Solution

- Add a configurable default lat/lon in settings (advanced section)
- If no location is configured and geocoding fails, show "no weather data" instead of wrong weather
- Update the web UI to include location fields in the settings form

## Effort
~1 hour

## Labels
Phase 1 — Quick Wins
EOF
)" \
  '["enhancement", "ux"]'

create_issue \
  "Fix display face memory: replace raw new with static allocation or smart pointers" \
  "$(cat <<'EOF'
## Problem

`BGDisplayManager` creates 8 display face objects via raw `new` in `setup()` (lines ~38-52) that are never `delete`d. While not a practical leak (they live for the program lifetime), this is sloppy for an embedded system where every byte of heap matters and contributes to heap fragmentation.

## Proposed Solution

Either:
- Use `static` objects and store pointers to them (zero heap overhead)
- Use `std::array<std::unique_ptr<BGDisplayFace>>` for explicit ownership

## Effort
~30 minutes

## Labels
Phase 1 — Quick Wins
EOF
)" \
  '["enhancement"]'

# ─── Phase 2: CI/CD Hardening ───────────────────────────────────────

create_issue \
  "Add clang-format enforcement to CI" \
  "$(cat <<'EOF'
## Problem

A `.clang-format` file exists but nothing in CI enforces it. Style can drift over time without anyone noticing.

## Proposed Solution

Add a GitHub Actions step in `build.yaml` that runs:

```bash
clang-format --dry-run --Werror src/**/*.cpp src/**/*.h lib/**/*.cpp lib/**/*.h
```

Fails the build on any formatting violation.

## Effort
~30 minutes

## Labels
Phase 2 — CI/CD Hardening
EOF
)" \
  '["enhancement", "ci"]'

create_issue \
  "Add static analysis (cppcheck or clang-tidy) to CI" \
  "$(cat <<'EOF'
## Problem

No static analysis runs in CI. Null dereferences, unused variables, and performance anti-patterns can slip through unnoticed.

## Proposed Solution

Add a CI step that runs:

```bash
cppcheck --enable=warning,performance --error-exitcode=1 src/
```

Or alternatively, integrate `clang-tidy` with a `.clang-tidy` config targeting common embedded pitfalls.

## Effort
~1 hour

## Labels
Phase 2 — CI/CD Hardening
EOF
)" \
  '["enhancement", "ci"]'

create_issue \
  "Add firmware size budget check to CI" \
  "$(cat <<'EOF'
## Problem

Flash is at 73.9% and RAM at 17.6%. There is no automated early warning before hitting the memory wall. A new feature could silently push the firmware past usable limits.

## Proposed Solution

Add a CI step after the build that:
- Parses the PlatformIO build output for flash and RAM usage
- Fails the build if flash > 80% or RAM > 25%
- Posts the current usage as a build annotation for visibility

## Effort
~30 minutes

## Labels
Phase 2 — CI/CD Hardening
EOF
)" \
  '["enhancement", "ci"]'

# ─── Phase 3: Testing Foundation ────────────────────────────────────

create_issue \
  "Introduce host-side unit test framework" \
  "$(cat <<'EOF'
## Problem

There are zero automated tests. All validation is manual on physical hardware. This makes refactoring risky and regressions easy to miss.

## Proposed Solution

- Set up PlatformIO native test runner or ArduinoFake for host-side testing
- Create initial test targets for pure-logic classes:
  - `SettingsManager` — config parsing, validation, defaults
  - `BGSource` — BG value parsing, trend detection, data age calculation
  - Display face value formatting (mmol/L conversion, arrow rendering)
- Add a `test` step to CI that runs these on every push

## Effort
~4 hours for framework + initial tests

## Labels
Phase 3 — Testing
EOF
)" \
  '["enhancement", "testing"]'

create_issue \
  "Add integration tests for BG source parsers (patient safety critical)" \
  "$(cat <<'EOF'
## Problem

The BG source parsers (Nightscout, Dexcom, LibreLink Up, Medtrum, Medtronic/xDrip, local API) are the most critical code path in the project. Incorrect parsing means wrong blood glucose readings displayed to caregivers. There are no automated tests for any of them.

## Proposed Solution

- Capture real (anonymized) JSON responses from each CGM API
- Store as test fixtures in `test/fixtures/`
- Write tests that feed these fixtures through each parser and assert:
  - Correct BG value extraction
  - Correct trend arrow mapping
  - Correct timestamp/data age calculation
  - Correct handling of edge cases (missing fields, empty responses, API errors)
  - Correct unit conversion (mg/dL ↔ mmol/L)

## Priority
**HIGH** — This is the single most important test to add. Wrong readings are a patient safety issue.

## Effort
~4 hours

## Labels
Phase 3 — Testing
EOF
)" \
  '["enhancement", "testing"]'

create_issue \
  "Add web UI tests for config validation logic" \
  "$(cat <<'EOF'
## Problem

`script.js` contains form validation and settings serialization logic with no automated tests. Config errors could silently produce invalid device configuration.

## Proposed Solution

- Add a lightweight JS test runner (e.g. Vitest or Jest)
- Test `script.js` logic:
  - Form validation rules
  - Settings serialization/deserialization
  - Edge cases in input handling
- Integrate into CI

## Effort
~2 hours

## Labels
Phase 3 — Testing
EOF
)" \
  '["enhancement", "testing"]'

# ─── Phase 4: Security & Memory ─────────────────────────────────────

create_issue \
  "Encrypt sensitive credentials at rest in LittleFS" \
  "$(cat <<'EOF'
## Problem

WiFi passwords, Dexcom credentials, LibreLink credentials, and Medtrum credentials are stored in plaintext in `config.json` on the LittleFS flash partition. While the threat model is local-only, this is poor practice and could expose credentials if the flash is read externally.

## Proposed Solution

- Use ESP32 hardware AES or a lightweight crypto library
- Derive an encryption key from the device-unique eFuse ID (not hardcoded)
- Encrypt sensitive fields on write, decrypt on read
- Scope: WiFi password, Dexcom/LibreLink/Medtrum usernames and passwords
- Ensure backward compatibility: detect plaintext config on first boot after update and migrate

## Effort
~4 hours

## Labels
Phase 4 — Security & Memory
EOF
)" \
  '["enhancement", "security"]'

create_issue \
  "Make Dexcom application ID configurable" \
  "$(cat <<'EOF'
## Problem

The Dexcom application ID is hardcoded in `BGSourceDexcom.h`:
```
DEXCOM_APPLICATION_ID = "d89443d2-327c-4a6f-89e5-496bbb0317db"
```

If Dexcom rotates this ID, every deployed clock breaks until reflashed with new firmware.

## Proposed Solution

- Add an advanced setting field (hidden by default in the web UI) for the Dexcom application ID
- Use the current hardcoded value as the default
- Allow override without reflashing via the settings page
- Document the setting in the wiki for when it's needed

## Effort
~1 hour

## Labels
Phase 4 — Security & Memory
EOF
)" \
  '["enhancement"]'

create_issue \
  "Memory audit and optimization for ESP32 headroom" \
  "$(cat <<'EOF'
## Problem

The ESP32 is at 17.6% RAM and 73.9% flash. This leaves very little headroom for future features. Without understanding where memory goes and optimizing the largest consumers, the project will hit a hard wall.

## Proposed Solution

### Audit
- Profile heap usage over 24h with `ESP.getHeapSize()`, `ESP.getMinFreeHeap()`, `ESP.getMaxAllocHeap()`
- Identify the largest heap allocations (JSON docs, HTTP buffers, string copies)
- Map flash usage by section (code, data, LittleFS)
- Check for heap fragmentation patterns

### Optimization candidates
- Switch to ArduinoJson streaming/filter parser for large API responses (especially LibreLink Up at 527 LOC)
- Reduce HTTP receive buffer sizes where possible
- Use `F()` macro for remaining string literals to move them to flash
- Consider `PROGMEM` for static data (fonts are already in lib/)
- Evaluate if all 8 display faces need to be instantiated simultaneously

### Deliverable
- Document current memory map
- Implement top 3 optimizations by impact
- Set up ongoing monitoring (ties into firmware size budget CI check)

## Priority
**HIGH** — This determines the project's runway for future feature development.

## Effort
~8 hours

## Labels
Phase 4 — Security & Memory
EOF
)" \
  '["enhancement"]'

echo ""
echo "Done! Created 13 issues."
