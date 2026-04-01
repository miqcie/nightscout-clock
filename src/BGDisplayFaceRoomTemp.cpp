#include "BGDisplayFaceRoomTemp.h"

#include <math.h>

#include "BGDisplayManager.h"
#include "DisplayManager.h"
#include "SettingsManager.h"
#include "globals.h"

// Check if SHT31 sensor data is valid (sensor enabled, not NaN, within reasonable range)
static bool isSensorDataValid() {
    if (!SENSOR_READING) return false;
    if (isnan(CURRENT_TEMP) || isnan(CURRENT_HUM)) return false;
    // Reject clearly out-of-range values (raw Celsius)
    if (CURRENT_TEMP < -40.0f || CURRENT_TEMP > 125.0f) return false;
    if (CURRENT_HUM < 0.0f || CURRENT_HUM > 100.0f) return false;
    return true;
}

void BGDisplayFaceRoomTemp::showReadings(
    const std::list<GlucoseReading>& readings, bool dataIsOld) const {
    DisplayManager.clearMatrix();

    // Show BG reading on the right side (same as other faces)
    showReading(readings.back(), 31, 6, TEXT_ALIGNMENT::RIGHT, FONT_TYPE::MEDIUM, dataIsOld);
    showTrendVerticalLine(31, readings.back().trend, dataIsOld);

    if (isSensorDataValid()) {
        // Show room temperature on the left side
        float temp = CURRENT_TEMP;
        if (!IS_CELSIUS) {
            temp = temp * 9.0f / 5.0f + 32.0f;
        }

        // Color based on comfort: cyan<65F, green 65-80F, yellow 80-85F, red>85F
        // Always compute Fahrenheit from raw sensor value (Celsius) for consistent thresholds
        int tempF = (int)(CURRENT_TEMP * 9.0f / 5.0f + 32.0f);
        uint16_t tempColor;
        if (tempF < 65) tempColor = COLOR_CYAN;
        else if (tempF <= 80) tempColor = COLOR_GREEN;
        else if (tempF <= 85) tempColor = COLOR_YELLOW;
        else tempColor = COLOR_RED;

        char tempStr[6];
        snprintf(tempStr, sizeof(tempStr), "%d", (int)temp);

        DisplayManager.setTextColor(tempColor);
        DisplayManager.printText(0, 6, tempStr, TEXT_ALIGNMENT::LEFT, 2);

        // Draw humidity bar as a single pixel row at bottom
        int hum = (int)CURRENT_HUM;
        // Draw humidity bar: width proportional to humidity (0-100% mapped to 0-16px)
        int barWidth = hum * 16 / 100;
        for (int i = 0; i < barWidth; i++) {
            DisplayManager.drawPixel(i, 7, COLOR_BLUE);
        }
    } else {
        // Sensor disconnected or returning garbage — show placeholder
        DisplayManager.setTextColor(COLOR_GRAY);
        DisplayManager.printText(0, 6, "---", TEXT_ALIGNMENT::LEFT, 2);
    }

    BGDisplayManager_::drawTimerBlocks(readings.back(), MATRIX_WIDTH - 17, 18, 7);
}

void BGDisplayFaceRoomTemp::showNoData() const {
    DisplayManager.clearMatrix();

    if (isSensorDataValid()) {
        float temp = CURRENT_TEMP;
        if (!IS_CELSIUS) {
            temp = temp * 9.0f / 5.0f + 32.0f;
        }

        char tempStr[8];
        snprintf(tempStr, sizeof(tempStr), "%d*", (int)temp);

        int hum = (int)CURRENT_HUM;
        char humStr[6];
        snprintf(humStr, sizeof(humStr), "%d%%", hum);

        DisplayManager.setTextColor(COLOR_GREEN);
        DisplayManager.printText(0, 6, tempStr, TEXT_ALIGNMENT::LEFT, 2);

        DisplayManager.setTextColor(COLOR_BLUE);
        DisplayManager.printText(31, 6, humStr, TEXT_ALIGNMENT::RIGHT, 2);
    } else {
        // Sensor disconnected or returning garbage — show placeholder
        DisplayManager.setTextColor(COLOR_GRAY);
        DisplayManager.printText(0, 6, "---", TEXT_ALIGNMENT::LEFT, 2);

        DisplayManager.setTextColor(COLOR_GRAY);
        DisplayManager.printText(31, 6, "--%%", TEXT_ALIGNMENT::RIGHT, 2);
    }
}
