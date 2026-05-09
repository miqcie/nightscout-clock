# Nightscout Clock — Setup Guide

A step-by-step walk-through for parents setting up a Nightscout Clock for the first time. No prior firmware, ESP32, or Nightscout knowledge needed.

If something doesn't work, jump to [Troubleshooting](#troubleshooting) at the bottom.

## What you need

- **A Nightscout Clock device** — a [Ulanzi TC001](https://www.ulanzi.com/products/ulanzi-pixel-smart-clock-2882?aff=1191), which runs about $50 and is available in the US and Europe.
- **The USB-C cable** that came with the clock.
- **A computer with Google Chrome or Microsoft Edge.** Other browsers cannot flash the firmware. (Phones won't work for the first step either — you need a desktop browser.)
- **Your home Wi-Fi network name and password.**
- **Your child's CGM credentials.** Pick one:
  - **Dexcom**: the *sensor wearer's* Dexcom username and password (the child's account, not a follower account).
  - **LibreLink Up**: email and password.
  - **Medtrum EasyFollow**: email and password.
  - **Nightscout**: the URL of the Nightscout site (e.g. `https://mybloodsugar.herokuapp.com`) and the API secret.

About 10–15 minutes total.

## Step 1 — Flash the firmware

1. Plug the clock into your computer with the USB-C cable. The clock will turn on automatically.
2. Open Chrome (or Edge) and go to **<https://miqcie.github.io/nightscout-clock/>**.
3. Click **Install**.
4. A dialog will ask which device to connect to. Pick the one labeled `USB Serial` or `CH340` (the exact name varies). Click **Connect**.
5. Wait — the page shows a progress bar. The flash takes about a minute. Don't unplug the cable until it says *Installation complete*.
6. The clock will reboot on its own.

## Step 2 — Connect to the clock

After the reboot, the clock acts as its own Wi-Fi network for the first setup.

1. On your **phone**, open Wi-Fi settings.
2. Connect to the network named **`nsclock`**. There is no password.
3. The phone may show a warning like *"This network has no Internet"* — that's expected, ignore it.
4. Most phones will pop up the setup page automatically. If yours doesn't, open a browser and go to **<http://10.0.0.123/>**.

## Step 3 — Run the setup wizard

The setup page asks for a small set of essentials:

1. **Wi-Fi network name and password** — your home network, the one the clock will use for everything from now on.
2. **ZIP code** (or postal code) — used to set the timezone and to fetch local weather. One field gives both.
3. **CGM data source** — pick Dexcom, LibreLink Up, Medtrum, or Nightscout. Enter the credentials.
4. Tap **Connect**.

The clock will reboot and join your home Wi-Fi.

## Step 4 — You're done

Within about two minutes the display will show your child's blood glucose value with a trend arrow. If you don't see data right away, give it a full five minutes — the first reading takes a moment to fetch.

## Customizing the clock

Once it's working, there's a fuller settings page on the clock itself:

1. On the clock, the IP address scrolls across the display once at boot — write it down. (If you missed it, your router's admin page lists it under connected devices, or you can press the *select* button — the middle one — once the clock is showing data and the IP appears.)
2. From any device on the same Wi-Fi, open `http://<that-ip>/` in a browser.
3. From there you can:
   - Change which display face is shown by default.
   - Pick which faces auto-rotate (and how often).
   - Set glucose thresholds (low / urgent low / high / urgent high).
   - Configure audible alarms.
   - Adjust brightness manually or let it follow ambient light.
   - Set a **Device PIN** to lock the settings page so the kids can't poke around.

The buttons on the clock itself: `<` / `>` switch faces, the middle button toggles the display on and off (double-click).

## Troubleshooting

**The clock is still showing "nsclock" Wi-Fi after I configured it.**
The Wi-Fi password didn't take. Reconnect to `nsclock` and try again — the password field is the most common culprit.

**The display says "To API" and never gets data.**
The clock connected to Wi-Fi but can't reach your CGM provider. Most often: wrong username/password. Open `http://<clock-ip>/` from your phone and re-enter credentials. For Dexcom, make sure you're using the *sensor wearer's* account, not a follower account.

**Weather face shows a gray "---".**
ZIP code geocoding failed (or no ZIP was set). The clock will not display random weather as a fallback — it shows no-data instead. Open `http://<clock-ip>/` and check the ZIP field.

**I want to start completely over.**
Hold the *center* button while the clock boots (during the version-number screen). It will reset to factory defaults and create the `nsclock` Wi-Fi again.

**The clock is stuck on the version number, never starts.**
Plug it back into your computer and re-flash from <https://miqcie.github.io/nightscout-clock/>. Flashing is non-destructive — your settings will return after the next setup.

**The display is way too bright (or too dim) at night.**
Settings page → Device settings → set Brightness mode to "auto, dimmed at night" or to manual with your preferred level. The auto mode reads the ambient light sensor on the clock.
