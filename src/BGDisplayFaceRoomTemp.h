#ifndef BGDISPLAYFACEROOMTEMP_H
#define BGDISPLAYFACEROOMTEMP_H

#include "BGDisplayFaceTextBase.h"
#include "BGSource.h"

class BGDisplayFaceRoomTemp : public BGDisplayFaceTextBase {
public:
    void showReadings(const std::list<GlucoseReading>& readings, bool dataIsOld = false) const override;
    void showNoData() const override;
};

#endif
