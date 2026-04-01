#include "BGDisplayFaceBigText.h"

#include "BGDisplayManager.h"
#include "globals.h"

void BGDisplayFaceBigText::showReadings(
    const std::list<GlucoseReading>& readings, bool dataIsOld) const {
    showReading(readings.back(), 0, 7, TEXT_ALIGNMENT::LEFT, FONT_TYPE::LARGE, dataIsOld);

    // show trend as a vertical line on the rightmost column
    // (the 5x5 arrow bitmap overlaps the 7-row tall big font, so use
    // the single-column indicator instead — same pattern as Clock face)
    showTrendVerticalLine(MATRIX_WIDTH - 1, readings.back().trend, dataIsOld);
}
