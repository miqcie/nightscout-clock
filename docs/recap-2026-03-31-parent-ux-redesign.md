# Session Recap: Nightscout Clock Parent UX Redesign

**Date:** 2026-03-31
**Branch:** `feature/setup-wizard`
**Duration:** ~4 hours
**Repo:** miqcie/nightscout-clock (fork of ktomy/nightscout-clock)

---

## What was built

Transformed a developer-oriented CGM clock firmware into something approaching a consumer product. Eight concrete deliverables:

1. **v0.29.0 merge** from upstream (icon cleanup, lib reorg, Medtronic info)
2. **Three upstream PR branches** extracted from the rejected PR #143 — fs-boot-check, captive-portal, dexcom-ux — each focused and ready to submit per maintainer feedback
3. **Single-page captive portal** — WiFi network scan + CGM source selection + credential entry, all in one server-rendered HTML form with zero JavaScript. CSS `:checked` selectors handle the accordion show/hide.
4. **`/api/setup` endpoint** — processes the combined form POST, saves everything, restarts
5. **Auto-rotate display faces** — 15-second default cycle through all faces, button press overrides and resets timer
6. **Room temperature face** — shows indoor temp from SHT31 sensor with comfort-based color coding (cyan/green/yellow/red)
7. **Weather face** — fetches outdoor conditions from Open-Meteo API (no API key required), 30-minute cache, weather-code-based colors
8. **AP IP changed** from 192.168.4.1 to 10.0.0.1 — shorter, easier fallback for parents

---

## The Good

**The captive portal architecture is right.** The single-page form inside the captive portal WebView bypasses Safari's HTTPS-Only mode entirely. Parents never need to type an IP address or fight browser security settings. WiFi networks show up as tappable radio buttons. The CSS-only accordion (no JavaScript) works in iOS's crippled CaptiveNetworkSupport WebView. This is the hardest design problem we solved and it's solid.

**The council pattern worked.** When we hit the "should we do multi-page captive portal?" question, spinning up three agents (ESP32 feasibility, iOS behavior research, UX alternatives) produced a clear answer in ~90 seconds: single page, under 60 seconds, don't fight iOS. The iOS agent uncovered that the captive portal sheet can auto-dismiss after 30-60 seconds and the "Done" button can't be intercepted — knowledge that would have cost hours of debugging on-device.

**Open-Meteo was a great find.** No API key means zero parent configuration for weather. The 600-byte JSON response is trivially parseable on ESP32. This is the kind of "invisible feature" that makes a product feel polished.

**The Tufte/Mondrian aesthetic translated.** The dark captive portal page with bottom-border inputs, gray-to-white selected states, and no decorative elements looks intentional and clean on a phone. Not default Bootstrap, not Silicon Valley startup — it looks like a medical device config page should look.

---

## The Bad

**The old web UI is a landmine.** The existing `index.html` with its Bootstrap cards and jQuery-based save flow coexists poorly with the new setup architecture. The "Validate, save and reset" button reads from old form fields that may be empty, overwriting valid credentials. This caused multiple frustrated restart cycles. Issue #7 is the most important fix before any demo.

**Flash/restart cycle is punishing.** Every UI change to the server-rendered captive portal requires: edit C++ string literals → build → flash firmware → wait 30s → test. No hot reload. The local preview HTML files helped for CSS/layout iteration, but the server-rendered page diverges from the preview as the C++ string concatenation gets complex. Need a better dev loop — maybe a Python mock server that mimics the ESP32 endpoints.

**CH340 USB-serial chip is unreliable.** Flash failures happened ~30% of the time. The chip drops connection at 921600 baud (had to lower to 460800), sometimes needs unplug/replug, and the serial port name changes between `/dev/cu.usbserial-10` and `/dev/cu.usbserial-110` unpredictably. This is a known Ulanzi TC001 hardware issue.

**The compound command hook bit us early.** I chained commands with `;` despite the existing feedback rule. Old habits in new sessions.

---

## The Ugly

**iOS password manager access is broken by design.** When a parent switches to 1Password during captive portal setup, iOS drops the `nsclock` WiFi connection because there's no internet. The parent has to reconnect and start over. There's no good fix within the current architecture — it's a fundamental iOS behavior with captive portal networks. Filed as issue #1 but no clear path to resolution beyond "use a laptop."

**WiFi.scanNetworks() was silently killing the captive portal.** The synchronous 3-second scan blocked the AP radio, causing every captive portal page load to disconnect the client. This manifested as "stuck on redirecting to setup" with no error message. Took multiple debug cycles to identify because we couldn't get serial output from within the Claude Code terminal. Fixed by caching the scan at boot.

**The firmware + filesystem coupling is fragile.** Adding new settings fields (face_auto_rotate, face_rotate_interval_sec) to the firmware without reflashing the filesystem causes a crash (angry red LED). The old config.json lacks the new fields. ArduinoJson returns defaults for missing keys, so the crash is likely in the new face class constructors, not the settings loader. This needs a config version check or more defensive initialization. Filed as #6.

**The "captive portal → Safari handoff" problem nearly derailed the whole session.** We spent significant time designing a two-page architecture (captive portal for WiFi, Safari for everything else) before discovering that Safari's HTTPS-Only mode blocks HTTP connections to local devices. The pivot to single-page captive portal was the right call but only happened after the council debate. Earlier discovery of the HTTPS-Only constraint would have saved an hour.

---

## Key Decisions (for future reference)

| Decision | Why | Alternative rejected |
|----------|-----|---------------------|
| Single-page captive portal | iOS kills portal after ~60s, no JS available | Multi-page wizard (timeout risk) |
| CSS :checked accordion | No JavaScript in iOS CaptiveNetworkSupport | JS-based show/hide |
| Open-Meteo API | No API key = zero parent config | OpenWeatherMap (requires API key signup) |
| Cached WiFi scan at boot | Per-request scan kills AP radio | Async scan (more complex, same result) |
| 10.0.0.1 AP IP | Shorter, easier to type as fallback | 192.168.4.1 (default, 12 chars) |
| Don't test WiFi mid-flow | AP+STA channel switching kills captive portal | Test-then-proceed (channel conflict) |

---

## What's next

**Tonight (priority order):**
1. Fix #7 — old save button overwriting WiFi credentials
2. Fix #2 — face rotation selection (choose which faces to include)

**This week:**
3. #3 — Weather location settings (currently hardcoded to Richmond VA)
4. #5 — Submit 3 focused PRs to upstream ktomy/nightscout-clock
5. #6 — Config version check to prevent firmware/filesystem mismatch crash

**Later:**
6. #4 — Parent lock / credential protection
7. #1 — iOS password manager workaround (may require BLE or companion app)

---

## Open Issues

| # | Title | Priority |
|---|-------|----------|
| 1 | iOS captive portal: can't access password managers | Medium |
| 2 | Auto-rotate: let users choose which faces to include | High |
| 3 | Weather face: add location settings (lat/lon) | Medium |
| 4 | Parent lock: credential protection | Low (deferred) |
| 5 | Submit 3 focused PRs to upstream | Medium |
| 6 | Filesystem reflash required after firmware update | High |
| 7 | Old web UI save overwrites WiFi credentials | Critical |
