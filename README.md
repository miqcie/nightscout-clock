# Nightscout Clock — parent-friendly fork

![Nightscout clock logo](https://github.com/ktomy/nightscout-clock/assets/1446257/1198c06d-b017-409d-aca3-2bca63581ecb)

This is **[miqcie/nightscout-clock](https://github.com/miqcie/nightscout-clock)**, a fork of [ktomy/nightscout-clock](https://github.com/ktomy/nightscout-clock) tuned for parents of T1D kids who want a glanceable bedside CGM display without becoming firmware engineers. The original project's features are intact; this fork prioritizes setup that a non-technical parent can complete in a few minutes from a phone.

> **Just looking to set one up?** See the [parent-friendly setup guide](docs/setup-guide.md). Web flasher: <https://miqcie.github.io/nightscout-clock/>

## What this fork adds on top of upstream

- **Captive-portal setup wizard** — clock creates an `nsclock` Wi‑Fi network, your phone auto-opens a setup page; enter Wi‑Fi + ZIP + CGM creds and you're done.
- **ZIP-code geocoding** — one ZIP gives you both the local timezone and weather coordinates without picking from a list.
- **8 display faces with auto-rotate selection** — Simple, Big Digits, Graph, Graph + value, Delta, Clock + value, Room temperature, Outdoor weather. Pick any subset; the clock cycles through the ones you enable.
- **Device PIN (parent lock)** — settings page can be locked behind a PIN so curious kids don't poke around.
- **SHT31 temperature sensor disconnect guard** — graceful handling when the optional temperature/humidity sensor isn't present or comes loose.
- **Web flasher** — first-time install runs entirely in Chrome via WebSerial; no PlatformIO required for end users.
- **Honest "no weather" UX** — when ZIP geocoding fails the weather face shows a clear no-data indicator instead of silently displaying weather for the wrong city.
- **Validated settings** — out-of-range BG thresholds, brightness, or alarm snoozes get clamped to safe defaults on load instead of wedging the device.

Display face docs live in [docs/display-faces.md](docs/display-faces.md).

## Hardware

[Ulanzi TC001](https://www.ulanzi.com/products/ulanzi-pixel-smart-clock-2882?aff=1191) (~$50, available in US and EU). The fork has not been tested on AWTRIX-Light hardware but the underlying firmware should work with minor changes — see upstream notes below.

## Building from source

PlatformIO + VS Code is the supported workflow. Clone the repo and run `pio run -e ulanzi` (release) or `pio run -e ulanzi_debug` (extra logs). Filesystem image: `pio run --target buildfs -e ulanzi`. Host-side unit tests: `pio test -e native`.

## Upstream attribution

This fork builds on [ktomy/nightscout-clock](https://github.com/ktomy/nightscout-clock) by [@ktomy](https://github.com/ktomy). All upstream credit and licensing is preserved; the rest of this README is the upstream project's documentation.

The fork diverges in a few places — most notably, OTA firmware updates and the captive-portal setup flow are deliberately different from upstream. PR #143 to upstream was rejected for OTA partition reasons; this fork keeps a single-app partition and accepts the bricking risk in exchange for headroom.

---

## Here is what it can do

- 6 colorful clockfaces
- Can get glucose data from Dexcom Share, Nightscout, LibreLink Up or Medtrum EasyFollow
- Supports mg/dl and mmol/l
- 10 minutes setup through web browser
- Configurable low/high limits
- Audible alarms in case the blood sugar is too low or too high
- Automatic brightness adjustment
- Notifies of stall data
- ...and more

### YouTube review

[![YouTube video](https://img.youtube.com/vi/7GmDflLxqLs/0.jpg)](https://www.youtube.com/watch?v=7GmDflLxqLs)

Thanks [@CallumMcK](https://github.com/CallumMcK)

## How to install

1. Buy [Ulanzi TC001](https://www.ulanzi.com/products/ulanzi-pixel-smart-clock-2882?aff=1191), it is about $50, so you don't have to sell a kidney, it is available both in US and Europe (Aliexpress sells it as well, for the same price)
2. Wait a few days for the delivery
3. Unpack, turn on (press on `<` and `>` buttons for a few seconds)
4. Connect the USB-C cable (comes with the clock) to your computer
5. Go to the [installation page](https://ktomy.github.io/nightscout-clock/)
6. Follow the instructions
7. Once the clock installed, take out your phone and join `nsclock` wi-fi network. Then go to `http://192.168.4.1/`
8. Set up your device, provide the Wi-Fi network details, your Dexcom, Nightscout, LibreLink Up or Medtrum EasyFollow credentials, glucose warning limits and other parameters
9. You're all set, enjoy!

## How to update

[![IMAGE ALT TEXT HERE](https://img.youtube.com/vi/7mFZJ7_EFN4/0.jpg)](https://www.youtube.com/watch?v=7mFZJ7_EFN4)

Thanks [@CallumMcK](https://github.com/CallumMcK)

## More information for people who needs it

Nightscout Clock is a custom firmware for Ulanzi TC001. It can also run (with minor changes) on AWTRIX-Light custom hardware, so if you need a bigger display, feel free to research.

### Clockfaces

| Name            | Look                                                                                                 | Comment |
| --------------- | ---------------------------------------------------------------------------------------------------- |---------|
| Simple          | <img width="500" alt="Simple" src="https://github.com/user-attachments/assets/ad281e9f-8c7f-41ff-ba82-23c634171158" /> |   Horizontal bars in the bottom of the display <br /> indicate the time since the last reading <br />No bars: less than one minute <br /> 1..5 green bars: 1..5 minutes <br /> 5 yellow bars: 6..20 minutes <br /> gray-colored value and vars: 20+ minutes       |
| BIG DIGITS      | <img width="500" alt="Big Digits" src="https://github.com/user-attachments/assets/1feae65b-21e9-4c20-8960-b75583baa142" /> |         |
| 3-hours graph   | <img width="500" alt="graph" src="https://github.com/user-attachments/assets/45d92097-f459-44d4-b1ae-a35c3cb38700" /> |         |
| Graph and value | <img width="500" alt="Graph and value" src="https://github.com/user-attachments/assets/db9046aa-5121-43fa-b367-807cdf3c5ef3" /> |  The dots on the right side replace the trend arrow.<br>2 white dots = horizontal arrow.<br>2 colored dots (white + green) = 45° arrow.<br>3 dots = vertical arrow.<br>4 dots = double arrow.<br>Colored dots above = upward trend.<br>Colored dots below = downward trend. <br /><br /> Dots under the value are the same as <br /> horizontal bars on the other faces.<br /> See "Simple" face for details |
| Delta           | <img width="500" alt="Photo of the Nightscout Clock" src="https://github.com/user-attachments/assets/f8005f49-6e32-43f1-bd84-0bb4e4691d7f" /> |         |
| Time and value  | <img width="500" alt="Time and value" src="https://github.com/user-attachments/assets/cd72bf15-85e3-4621-b5ca-d639c1849cd5" /> | The dots on the right side replace the trend arrow.<br>2 white dots = horizontal arrow.<br>2 colored dots (white + green) = 45° arrow.<br>3 dots = vertical arrow.<br>4 dots = double arrow.<br>Colored dots above = upward trend.<br>Colored dots below = downward trend. <br /><br /> For the bottom-side bars see "Simple" face for details |

### Configuration web interface

<img alt="webUI" src="https://github.com/user-attachments/assets/94222c87-3f96-46f9-a773-02f7cdb16e6b" />

### Features (technical stuff, feel free to ignore)

- Web-based installation (no need to install flashing tools, you just need the clock and a web browser). The clock hosts a website where the user can configure all the parameters (data source, limits, alarms, display)
  - Access-Point mode to set the initial configuration
  - Support for a secondary WiFi (e.g. if you want to take the clock in your car for a long trip)
  - Support for WPA-Enterprise
  - Ability to set a custom hostname in case you have multiple NSClocks on the same network
- Simple glucose value display with trend arrow
- Changing color based on limits
- Nightscout data source, the clock gets units type and value boundaries from Nightscout (see [how to](https://youtu.be/GGiep2gdx_o) set up using [Nightscout.pro](https://www.nightscout.pro/) as data source)
- [Juggluco](https://www.juggluco.nl/) data source (support for HTTP Nightscout endpoints)
- [Improve WiFi](https://github.com/improv-wifi) compatibility (setting up WiFi during the installation)
- [Gluroo](https://gluroo.com/) data source (API_SECRET within the URL parameters) (see how to setup [video](https://youtu.be/unG-l6XXWxw))
- Simplified Nightscout API like xDrip+ [Open Web Service](https://github.com/NightscoutFoundation/xDrip/blob/master/Documentation/technical/Local_Web_Services.md) support. When adding it as a data source, choose Nightscout and check the `Simplified API` checkbox. Make sure your source device (e.g. your phone running xDrip) has a static IP address and is on the same WiFi network. Also don't forget to check the port setting; for xDrip it is usually `17580`
- Dexcom Share data source
- LibreLinkUp (libreview) data source
- Medtrum EasyFollow data source
- Medtronic users can bridge data through xDrip+ to Nightscout, then connect the clock to that Nightscout site. See [discussion #53](https://github.com/ktomy/nightscout-clock/discussions/53)
- Brightness adjustment
  - Brightness can be adjusted within the Web UI
  - Automatic brightness adjustment based on the ambient light
  - Double-click on the middle button on the clock turns the display on and off
- Multiple clock faces support
  - Default clock face can be selected in the Web UI
  - Clock faces can be changed using arrow buttons on the clock
  - Simple clock face (value and trend arrow)
  - Full-width glucose graph
  - Graph, value and trend indicator
  - BIG DIGITS
  - Value, trend and delta
  - Clock and BG value (timezone is set in the clock's web interface)
- Changes color to gray if the data is too old
- Smart data and screen update timings: read data once it appears, refresh screen when needed
- API data source. The clock has a simple Nightscout-like API which can receive glucose values from an external source. The main purpose of this feature is the ability to test the clock during the clockfaces development. In order to activate this feature, select the API data source within the clock's Web UI. Here are the endpoints:
  - /api/v1/entries POST endpoint receives an array of Nightscout-like entries. The only significant fields are `sgv`, `date` and `trend` or `direction`. Due to the limited memory the API is stable when sent less than 10 recotds
  - /api/v1/entries DELETE endpoint deletes all entries regardless of the payload
- Firmware versioning
- Alarms with configurable Thresholds, snooze times and silence intervals
- To turn the device on or off press both arrow buttons for 3 seconds
- To reset the device to factory defaults (hard reset) during boot sequence (when version number is displayed) keep the center (select) button pressed.

### My TODO list

- Add more clock faces
  - Battery, humidity and temperature
- Smooth color change (rainbow) based on the value and boundaries
- Add more data sources
  - ...more... (if you are the author of a CGM data collecting app/service and you want your data to be displayed on the Nightscout Clock, please contact me)

## Changes

### 0.29

- Added Medtronic setup guidance to the WebUI, closes [#147](https://github.com/ktomy/nightscout-clock/issues/147)
- Added web auth in the advanced settings. ([discussion](https://github.com/ktomy/nightscout-clock/issues/72)), thank you [@mrendo](https://github.com/mrendo)
- Added show password option on all password fields, thank you [@mrendo](https://github.com/mrendo)
- Fixed local WebUI password visibility icons after the changes from [#125](https://github.com/ktomy/nightscout-clock/pull/125)

### 0.28

- Fixed email parsing Medtrum credentials input, thanks [@logicafuzzy](https://github.com/logicafuzzy), merged [#127](https://github.com/ktomy/nightscout-clock/pull/127)
- Added support for xDrip+ [Open Web Service](https://github.com/NightscoutFoundation/xDrip/blob/master/Documentation/technical/Local_Web_Services.md) as an extension of the Nightscout data source, resolving [#86](https://github.com/ktomy/nightscout-clock/discussions/86)

### 0.27

- Medtrum EasyFollow data source support ([discussion](https://github.com/ktomy/nightscout-clock/discussions/106)), inspired by [@pachi81](https://github.com/pachi81)'s work
- Fixed Open WiFi networks validation
- Moved trend indicator to the right in the "Time and value" clockface

### 0.26.2

- Implemented LibreLink Up patient selection for multi-patient accounts. Fixed [#65](https://github.com/ktomy/nightscout-clock/issues/65)

### 0.26.1

- Fixed a typo in the WebUI javascript

### 0.26.0

- Added trend indicator to the clock-and-value clock face. [#49](https://github.com/ktomy/nightscout-clock/issues/49)
- Added manual brightness control from the device itself. When the brightness is in manual mode, pressing `<` or `>` for more than a second increases or decreases brightness. Setting persists over restarts. [#74](https://github.com/ktomy/nightscout-clock/issues/74)

### 0.25.2

- Fixed support for Dexcom Japan
- Added custom alarm melodies, which can be set through RTTTL strings in alarm settings

### 0.25.1

- Added support for Dexcom Japan: [#110](https://github.com/ktomy/nightscout-clock/discussions/110)
- Fixed typo in Dexcom credentials error message: [#111](https://github.com/ktomy/nightscout-clock/issues/111)

### 0.25.0

- Implemented Alarm Intensive Mode, addressing [#61](https://github.com/ktomy/nightscout-clock/discussions/61)

### 0.24.4

- Fixed WiFi password validation for Open WiFi networks
- Added possibility of factory resetting the device on start-up (when version string is displaying press the select button)
- Better AP mode handling

### 0.24.3

- Implemented open WiFi connectivity, closing [#105](https://github.com/ktomy/nightscout-clock/issues/105)
- Changed age bars colors so that they do not disappear when the brightness is minimal. [#101](https://github.com/ktomy/nightscout-clock/discussions/101)

### 0.24.2

- Increased version presented to LibreLinkUp servers, closing [#88](https://github.com/ktomy/nightscout-clock/issues/88)

### 0.24

- Added "dark rooms" brightness mode and changed curve for the manual brightness. Thanks [@unxmaal](https://github.com/unxmaal) for [#67](https://github.com/ktomy/nightscout-clock/pull/67)

### 0.23.1

- Fixed trend arrow display on the Big Text clockface when brightness is not in auto mode. [#47](https://github.com/ktomy/nightscout-clock/issues/47)
- Adjusted version display on start-up

### 0.23

- Added version number, update check and link to the updating page to the WebUI. [#10](https://github.com/ktomy/nightscout-clock/issues/10)
- Added development environment setup instructions. [#26](https://github.com/ktomy/nightscout-clock/issues/26)
- Fixed LibreLink Up Server label. [#50](https://github.com/ktomy/nightscout-clock/issues/50)

### 0.22

- Fixed delta for LibreLinkUp source
- Now I can consider LibreLinkUp as being functioning. So closing [#9](https://github.com/ktomy/nightscout-clock/issues/9) and [#27](https://github.com/ktomy/nightscout-clock/issues/27)

### 0.21

- Yet another attempt to fix LibreLinkUp

### 0.20

- Another attempt to get values from LibreLinkUp

### 0.19

- Added custom "Data is old" threshold, (thanks [@TheBeardedTechGuy](https://github.com/TheBeardedTechGuy) for [#43](https://github.com/ktomy/nightscout-clock/pull/43))
- Added glucose age display as bars in the bottom part of most of the clockfaces (thanks [@TheBeardedTechGuy](https://github.com/TheBeardedTechGuy) for [#41](https://github.com/ktomy/nightscout-clock/pull/41))

### 0.18

- Fixed Nightscout domain name validation, closed [#39](https://github.com/ktomy/nightscout-clock/issues/39)

### 0.17

- Added API endpoint to switch display power on and off as in the example below

```
curl -X POST http://nsclock.lan/api/displaypower \
  -H "Content-Type: application/json" \
  -d '{"power": "off"}'
```

### 0.16

- Another attempt to fix [#25](https://github.com/ktomy/nightscout-clock/issues/25) and [#31](https://github.com/ktomy/nightscout-clock/issues/31)

### 0.15

- Fixed delta showing 0 when there are multiple sources uploading data to Nightscout. [#25](https://github.com/ktomy/nightscout-clock/issues/25)
- Implemented WPA-Enterprise connectivity (Wifi type selector available for additional WiFi network). [#23](https://github.com/ktomy/nightscout-clock/issues/23)
- Fixed project dependencies divergence. [#30](https://github.com/ktomy/nightscout-clock/issues/30)
- Implemented possibility of setting custom hostname. [#18](https://github.com/ktomy/nightscout-clock/issues/18)
- Implementing possibility of having additional WiFi network. [#7](https://github.com/ktomy/nightscout-clock/issues/7)

### 0.14

- Updated README to be more customer-friendly
- Started preparing for LibreLink Up data source integration, [#9](https://github.com/ktomy/nightscout-clock/issues/9))
- Improved memory management to avoid NOMEMORY issues and unplanned restarts, [#19](https://github.com/ktomy/nightscout-clock/issues/19)
- Improved WiFi SSID validation, [#16](https://github.com/ktomy/nightscout-clock/issues/16)
- Fixed typos in Web UI (thanks @motinis)

### 0.13

- Finished alarms feature
- Improved WiFi reconnection handling [#15](https://github.com/ktomy/nightscout-clock/issues/15)
- Migrated development environment to Linux
- Removed boot sound

## Do you want to contribute?

Please read the [Contriburing](https://github.com/ktomy/nightscout-clock/blob/main/CONTRIBUTING.md) instructions

---

The code is heavily inspired by (has a lot of copy-pasted code from :D ) [AWTRIX Light](https://github.com/Blueforcer/awtrix-light) project.
