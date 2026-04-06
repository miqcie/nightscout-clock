#ifndef SettingsManager_H
#define SettingsManager_H

#include <ArduinoJson.h>
#include <IPAddress.h>
#include <Preferences.h>
#include <Settings.h>

#include "enums.h"

class SettingsManager_ {
private:
    SettingsManager_() = default;
    JsonDocument* readConfigJsonFile();
    Preferences nvsPrefs;
    bool saveCredentialsToNVS();
    bool loadCredentialsFromNVS();

public:
    static SettingsManager_& getInstance();
    bool setup();
    bool loadSettingsFromFile();
    bool saveSettingsToFile();
    bool trySaveJsonAsSettings(const JsonDocument& doc);
    void factoryReset();
    bool restoreConfigFromNVS();
    bool recreateDefaultConfig();
    void clearNVS();

    Settings settings;
};

extern SettingsManager_& SettingsManager;

#endif