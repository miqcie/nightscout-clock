# Session Recap: 2026-04-01

## What shipped

**12 PRs merged** to miqcie/nightscout-clock in a single session:

| PR | Feature |
|----|---------|
| #15 | Filesystem self-check on boot |
| #16 | Captive portal detection (iOS/Android/Windows) |
| #17 | Dexcom credential hints + password autocomplete |
| #18 | Full parent-friendly setup wizard: single-page captive portal, auto-rotate, weather + room temp faces, zip code geocoding, plus 16 review-driven fixes (XSS, save-restart race, double Fahrenheit conversion) |
| #19 | WiFi credential overwrite protection |
| #20 | SHT31 sensor disconnect guard |
| #21 | Big text face trend arrow |
| #22 | Auto-rotate face selection (user-configurable) |
| #23 | Face simulator redesign with LED glow effects |
| #24 | Parent lock (Device PIN in captive portal) |
| #25 | Display face documentation |
| #26 | GitHub Pages web flasher with CI/CD pipeline |

**13 of 14 issues closed.** Only #8 (OTA firmware updates) remains — full implementation plan written, deferred because it requires a partition table change and USB reflash for all existing users.

**3 upstream PRs submitted** to ktomy/nightscout-clock (per their feedback on closed PR #143):
- ktomy#152: Captive portal detection
- ktomy#153: Filesystem self-check on boot
- ktomy#154: Dexcom UX hints + JSON escape

**Web flasher deployed** at https://miqcie.github.io/nightscout-clock/

## Architecture decisions

- **Appliance over platform.** Evaluated AWTRIX 3 as an alternative firmware. Rejected — parents need a single-device appliance, not a Home Assistant dependency. Study AWTRIX's OTA pattern for our own implementation.
- **Zip code as universal input.** One captive portal field resolves weather location + timezone via Open-Meteo geocoding after WiFi connects. IANA-to-POSIX mapping for US timezones.
- **Flag-based deferred restart.** Replaced all `delay() + ESP.restart()` patterns in async web handlers with `pendingRestart` flag checked in `tick()`. Response flushes before reboot.
- **CSS-only captive portal.** No JavaScript in the setup form — iOS captive portal WebViews may block it. CSS `:checked` selectors handle show/hide.

## Review pipeline

Ran 3 specialized review agents in parallel on PR #18:
1. **Code reviewer** — bugs, security, hardcoded values
2. **Silent failure hunter** — swallowed errors, race conditions, missing null checks
3. **Comment analyzer** — accuracy, misleading comments, missing context

First pass found 16 issues. Second pass (post-fix) found 3 more criticals in the fix code itself:
- `/api/factory-reset` missed in the delay-to-flag conversion
- `resolveZipLocation()` permanently gave up on transient failures
- `saveSettingsToFile()` return value unchecked in new code (ironic — that was the exact first-round finding)

## Process improvements

Two feedback rules created from review mistakes:
1. **grep-all-pattern-instances** — After fixing a pattern, grep for ALL instances before committing
2. **apply-fixes-to-own-code** — Apply review lessons to new code in the same commit, not just old code

## Build stats

- RAM: 17.6% (57,636 / 327,680 bytes)
- Flash: 73.9% (1,549,377 / 2,097,152 bytes)

## What's next

- **#8 OTA firmware updates** — Plan complete (partition table redesign, HTTP upload endpoints, boot validation + rollback, web UI). Requires one-time USB reflash for all existing users to adopt new dual-OTA partition layout.
- **#27 Web flasher redesign** — Restyle to match cmm.dev aesthetic (Mondrian grid, monospace, terminal-inspired)
- **#5 upstream PRs** — Monitor ktomy's review of #152, #153, #154
- **Hardware testing** — Flash to a physical TC001 and test the full setup flow end-to-end
