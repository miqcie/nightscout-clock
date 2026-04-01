# Display Faces Reference

The Nightscout Clock has 8 display faces rendered on a 32x8 pixel LED matrix. Each face shows blood glucose (BG) data in a different layout, and some include additional information like graphs, clocks, room temperature, or weather.

Faces can be cycled via the device button or selected in the web settings. You can also restrict which faces appear in the rotation using the "enabled faces" setting.

---

## Table of Contents

- [0 - Simple](#0---simple)
- [1 - Full Graph](#1---full-graph)
- [2 - Graph and BG](#2---graph-and-bg)
- [3 - Big Text](#3---big-text)
- [4 - Value and Diff](#4---value-and-diff)
- [5 - Clock and Value](#5---clock-and-value)
- [6 - Room Temp](#6---room-temp)
- [7 - Weather](#7---weather)
- [Common Elements](#common-elements)

---

## 0 - Simple

**Shows:** BG value (medium font, centered), trend arrow, staleness timer

The default face. BG reading is centered on the display with a 5x5 trend arrow on the right side. Timer blocks along the bottom-right corner indicate how old the reading is.

```
+--------------------------------+
|                                |
|          1 2 0      >>>        |
|                     >>>        |
|                     >>>        |
|                     >>>        |
|                     >>>        |
|          [mg/dL]               |
|                          [tmr] |
+--------------------------------+
 Col 0              Col 27  Col 31
```

**Best for:** Quick glance at current BG. The large trend arrow makes direction easy to read from across the room.

---

## 1 - Full Graph

**Shows:** Sparkline graph of recent BG readings across the entire 32-pixel width

The full display is a graph showing the last 180 minutes (3 hours) of BG history. Each column is one reading (~5 min apart). Pixel height maps to BG range. No numeric value is shown.

```
+--------------------------------+
|      *                         |
|     * *                        |
|    *   *          *            |
|   *     *       ** **          |
|  *       *    **     *         |
|           * **        **       |
|            *            *      |
|                          **    |
+--------------------------------+
 <--------- 180 min ----------->
```

**Best for:** Seeing trends and patterns over time. Useful when you care more about direction and stability than the exact number.

---

## 2 - Graph and BG

**Shows:** Sparkline graph (left), BG value (right, medium font), trend line, staleness timer

Combines a truncated graph on the left with the current BG reading and trend indicator on the right. The graph width adjusts based on how many characters the BG value needs (wider values shrink the graph). A single-column vertical trend line appears next to the BG value.

```
+--------------------------------+
|                                |
|    *                           |
|   * *          | 1 2 0         |
|  *   *    *    |               |
|       * **     |               |
|        *       |               |
|                |               |
|  [graph]         [tmr]         |
+--------------------------------+
 <--graph-->  trend  <--BG-->
```

**Best for:** Having it all -- recent history plus the current number at a glance.

---

## 3 - Big Text

**Shows:** BG value in large 7-row font (left-aligned), trend line (rightmost column)

The BG number fills nearly the entire display using a large font that spans all 7 usable rows. A single-pixel-wide vertical trend indicator occupies column 31 (the rightmost column).

```
+--------------------------------+
|                               ||
| #  ###   ###                  ||
| # #   # #   #                 ||
| #     # #   #                 ||
| #   ##  #   #                 ||
| #  #    #   #                 ||
| # #####  ###                  ||
|                               ||
+--------------------------------+
 <--- large BG value --->   trend
```

**Best for:** Maximum readability from a distance. Ideal for bedside use where you want to read the number from across the room without glasses.

---

## 4 - Value and Diff

**Shows:** BG value (left, medium font), trend arrow, delta/diff from previous reading (right), staleness timer

Shows the current BG on the left with a trend arrow, plus a signed difference value on the right (e.g., "+5" or "-12"). The diff is calculated from readings in the last 6.5 minutes. If the change is too large (>99) or erratic, a "?" is shown instead.

```
+--------------------------------+
|                                |
|          >>>                   |
| 1 2 0    >>>        + 5       |
|          >>>                   |
|          >>>                   |
|          >>>                   |
|  [BG]              [diff]     |
|                          [tmr] |
+--------------------------------+
 <--BG--> arrow    <--diff-->
```

**Best for:** Understanding not just where BG is, but how fast it is changing. The numeric delta makes it easy to judge urgency.

---

## 5 - Clock and Value

**Shows:** Current time (left, HH:MM), BG value (right, medium font), trend line, staleness timer

Displays the current time in the left half using medium font (hours and minutes). The right half shows the BG value with a vertical trend line. In 12-hour mode, a colored bar along the bottom-left indicates AM (cyan) or PM (blue).

```
+--------------------------------+
|                                |
|                       |        |
| 0 9  4 5        120  |        |
|                       |        |
| [HH] [MM]       [BG] |        |
|                       |        |
|                                |
| [AM/PM bar]           [tmr]   |
+--------------------------------+
 <--time-->        <--BG--> trend
```

**Best for:** Bedside overnight use. See the time and BG together without needing a separate clock.

---

## 6 - Room Temp

**Shows:** Room temperature (left), BG value (right, medium font), trend line, humidity bar, staleness timer

**Requires:** SHT31 temperature/humidity sensor connected to the device.

Displays the indoor temperature on the left side, color-coded by comfort level:
- Cyan: below 65 F
- Green: 65-80 F (comfortable)
- Yellow: 80-85 F (warm)
- Red: above 85 F

A blue pixel bar along the bottom-left shows relative humidity (width proportional to 0-100%). BG and trend are on the right. If no sensor is connected, the left side shows "---".

```
+--------------------------------+
|                                |
|                       |        |
|  7 2            120   |        |
|                       |        |
| [temp]          [BG]  |        |
|                       |        |
|                                |
| [humidity bar]         [tmr]   |
+--------------------------------+
 <-temp->          <--BG--> trend
```

**Best for:** Monitoring a child's room conditions alongside their BG. Useful for nurseries and bedrooms where temperature matters.

---

## 7 - Weather

**Shows:** Outdoor temperature (left), BG value (right, medium font), trend line, weather indicator, staleness timer

**Requires:** ZIP code configured in settings (uses Open-Meteo API for weather data).

Displays the current outdoor temperature on the left, color-coded by WMO weather conditions:
- Yellow: clear sky
- White: partly cloudy
- Gray: fog
- Blue: rain
- Cyan: snow
- Red: thunderstorm

A 3-pixel color indicator at the bottom-left reinforces the weather condition. Weather data is cached and refreshed every 30 minutes (expires after 2 hours of failed refreshes). If no location is configured, falls back to Richmond, VA.

```
+--------------------------------+
|                                |
|                       |        |
|  5 8            120   |        |
|                       |        |
| [temp]          [BG]  |        |
|                       |        |
|                                |
| [wx]                   [tmr]   |
+--------------------------------+
 <-temp->          <--BG--> trend
```

**Best for:** Quick glance at outdoor conditions alongside BG, so you know whether to grab a jacket on your way out the door.

---

## Common Elements

These UI elements appear across multiple faces:

### Trend Indicators

- **Trend arrow (5x5):** A directional arrow bitmap showing BG trend. Used by Simple and Value and Diff faces.
- **Trend vertical line (1px wide):** A single-column indicator at the edge of the BG value. Used by Graph and BG, Big Text, Clock, Room Temp, and Weather faces. Preferred when a 5x5 arrow would overlap large text.

### Staleness Timer Blocks

Small blocks drawn along row 7 (the bottom pixel row) that indicate how many minutes have passed since the last CGM reading. Appear on most faces.

### BG Color Coding

BG values are color-coded by range (configurable in settings):
- **Urgent low / Urgent high:** Red
- **Warning low / Warning high:** Yellow
- **Normal range:** Green
- **Stale data:** Gray/dimmed

### Font Sizes

- **Medium font:** Standard size used by most faces (fits within ~6 rows)
- **Large font:** 7-row tall font used by Big Text face for maximum visibility

### Units

All faces respect the configured BG unit setting (mg/dL or mmol/L). The mmol/L display includes a decimal point and may require more horizontal space.
