# OTA firmware update plan

> Status: **plan, not implemented.** This document is the design checked in for
> future execution. Tracking issues: [#8](https://github.com/miqcie/nightscout-clock/issues/8) (high-level approach),
> [#30](https://github.com/miqcie/nightscout-clock/issues/30) (this doc).

## Why this matters

Once a Nightscout Clock leaves the desk and lands on a child's nightstand — or
worse, ships to a different family — every firmware update means asking a parent
to dig out the USB cable, find a Chrome-equipped computer, and reflash. That is
exactly the friction the parent-UX of this fork is trying to remove. OTA closes
the loop: a parent updates from their phone, like every other appliance.

This is the single-largest UX gap between this fork and what a polished
consumer device would feel like.

## Background — why we diverge from upstream

Upstream PR [#143](https://github.com/ktomy/nightscout-clock/pull/143) was
rejected for two reasons that don't apply equally to this fork:

1. **Dual-OTA partition table eats too much flash headroom.** Upstream has a
   release model that values headroom for future feature work over OTA
   convenience.
2. **Firmware-only OTA is insufficient.** ktomy's specific feedback: the web UI
   in LittleFS evolves alongside the firmware, so any update path that only
   replaces `firmware.bin` will leave users with a working binary and a stale
   web UI (or vice versa). Both partitions must be updateable in one motion.

This fork accepts the headroom hit in exchange for cable-free updates and
commits to updating both firmware and filesystem in a single OTA operation.

## Recommended design

### Partition table — dual-slot OTA

Switch `partitions.csv` to a standard ESP32 dual-OTA layout with a writable
LittleFS region:

```
nvs,      data, nvs,     0x9000,  0x5000
otadata,  data, ota,     0xe000,  0x2000
app0,     app,  ota_0,   0x10000, 0x1A0000
app1,     app,  ota_1,   ,        0x1A0000
spiffs,   data, spiffs,  ,        0x80000
```

- ~1.625 MB per app slot (vs ~2 MB single-app today).
- 512 KB LittleFS (down from 1 MB). Current FS image is well under that.
- ESP32 bootloader handles the slot swap and rollback semantics natively;
  no recovery partition required.

**Constraint:** changing the partition table requires a one-time USB reflash
for every existing deployed clock. There is no in-place migration. We accept
this as a one-time cost — every user who's already deployed will need to
reflash once, after which OTA works forever.

### HTTP upload endpoints

Two endpoints on the existing async web server:

- `POST /api/update` — accepts `firmware.bin`, writes to the inactive app slot,
  validates header + checksum, marks for next-boot, replies with status.
- `POST /api/update-fs` — accepts `littlefs.bin`, writes to the SPIFFS region
  (LittleFS uses the SPIFFS partition slot), no slot swap needed.

Both must be PIN-locked when the device PIN is set. Multipart/form-data upload
with a streaming body — we cannot buffer a full firmware in RAM.

### Validation before commit

Before marking a slot active:

1. **Magic byte check** — the first byte of an ESP32 app image is `0xE9`. Reject
   anything else immediately.
2. **Size check** — must fit in the slot. Reject early.
3. **Checksum** — verify the hash embedded in the image. The ESP32 SDK exposes
   `esp_ota_end()` which does this; surface the result to the UI.

If any check fails, abort and leave the active slot untouched.

### Boot validation + automatic rollback

The ESP32 bootloader supports marking an app slot as "pending verification":

1. After OTA, mark the new slot pending and reboot.
2. On boot, the new firmware must call `esp_ota_mark_app_valid_cancel_rollback()`
   within N seconds of network and BG-source connectivity.
3. If the firmware crashes, hangs, or fails to call the validation API, the
   bootloader rolls back to the previous slot on next reset.

This makes a bricked OTA practically impossible — the worst case is one extra
reboot cycle.

### Web UI

A new page `/update` (linked from the settings page) with:

- File picker for `firmware.bin`.
- File picker for `littlefs.bin`.
- "Update both" button — uploads firmware first, then filesystem, then triggers
  a single reboot. Progress bar fed by `XHR.upload.progress` events.
- Post-update view: "Verifying new firmware…" → "Update successful" or
  "Rollback occurred — please reflash via USB."

### Auto-update check (optional, phase 2)

A background task that polls
`https://api.github.com/repos/miqcie/nightscout-clock/releases/latest` every 24h
and surfaces "update available" on the display + settings page. The actual
update remains user-triggered — never silent — so a parent always knows when
firmware changes.

### Recovery flow

If the device fails to boot any valid firmware (extremely unlikely with the
rollback path above, but possible if both slots are corrupted):

- The existing fs-boot-check already detects a missing/corrupt LittleFS and
  drops into AP mode with `nsclock` SSID.
- The captive portal on AP mode will offer a "reflash via USB" message with a
  link to the web flasher. No silent failure.

## Out of scope (intentionally)

- **Recovery partition.** Adds complexity for a case the dual-slot rollback
  already handles.
- **Firmware-only OTA.** ktomy is right; we update both or neither.
- **Background auto-install.** Updates remain user-triggered.

## Implementation order

1. Partition table change + one-time USB reflash (breaking change — call out
   prominently in release notes).
2. `/api/update` endpoint with magic-byte + size + checksum validation.
3. Boot validation + rollback wiring.
4. `/api/update-fs` endpoint.
5. Web UI page.
6. Auto-update polling (optional).

## Risks

- **Partition migration friction.** Every existing clock needs a one-time USB
  reflash. Document loudly, ship with a clear migration guide, accept the cost.
- **Headroom shrinks.** Each app slot drops from ~2 MB to ~1.625 MB. Today's
  firmware is at ~73.9% of 2 MB ≈ 1.48 MB; that fits, with ~150 KB headroom
  per slot. Track in CI via the budget check from
  [#38](https://github.com/miqcie/nightscout-clock/issues/38).
- **Failed update mid-flight.** Mitigated by (a) writing to the inactive slot
  only and (b) rollback. The active firmware is never touched until validation
  passes.
