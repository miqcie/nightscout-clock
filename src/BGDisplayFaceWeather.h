#ifndef BGDISPLAYFACEWEATHER_H
#define BGDISPLAYFACEWEATHER_H

#include "BGDisplayFaceTextBase.h"
#include "BGSource.h"

struct WeatherData {
    float temperature;
    int humidity;
    int weatherCode;  // WMO weather code from Open-Meteo
    unsigned long fetchedAtMs;
    bool valid;
};

class BGDisplayFaceWeather : public BGDisplayFaceTextBase {
public:
    void showReadings(const std::list<GlucoseReading>& readings, bool dataIsOld = false) const override;
    void showNoData() const override;

    static void fetchWeather();
    static void resolveZipLocation();
    static WeatherData cachedWeather;

private:
    void showWeather() const;
    static uint16_t weatherCodeToColor(int code);
};

#endif
