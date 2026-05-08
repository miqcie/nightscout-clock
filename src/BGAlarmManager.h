#ifndef BGAlarmManager_h
#define BGAlarmManager_h

#include <Arduino.h>

#include <vector>

struct AlarmData {
    int bottom;
    int top;
    unsigned long lastAlarmTime;
    int snoozeTimeMinutes;
    String silenceInterval;
    String alarmSound;
    bool isSnoozed;
};

class BGAlarmManager_ {
private:
    BGAlarmManager_() = default;
    std::vector<AlarmData> enabledAlarms;
    AlarmData* activeAlarm = nullptr;
    int alarmIntervalSeconds = 0;

public:
    static BGAlarmManager_& getInstance();
    void setup();
    void tick();
    void snoozeAlarm();
};

extern BGAlarmManager_& bgAlarmManager;
#endif
