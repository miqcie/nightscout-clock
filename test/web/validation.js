// Test-only mirror of the validation patterns + createJson body shape from
// data/script.js. The production code lives inside an IIFE and would require
// invasive refactoring to expose for tests. Instead we mirror the rules here
// and rely on the tests to catch drift — when these regexes or the createJson
// shape change in data/script.js, update this file in the same commit.
//
// Source of truth: data/script.js (lines ~21-36 for patterns, ~830-962 for
// createJson serialization).

export const patterns = {
    ssid: /^[\x20-\x7E]{1,32}$/,
    wifi_password: /^.{8,}$/,
    dexcom_username: /^.{6,}$/,
    dexcom_password: /^.{8,20}$/,
    ns_hostname:
        /(^(?:[0-9]{1,3}\.){3}[0-9]{1,3}$)|(^(?:[a-z0-9](?:[a-z0-9-]{0,61}[a-z0-9])?\.)+[a-z0-9][a-z0-9-]{0,61}[a-z0-9]$)/,
    ns_port: /(^$)|(.{3,5})/,
    api_secret: /(^$)|(.{12,})/,
    bg_mgdl: /^[3-9][0-9]$|^[1-3][0-9][0-9]$/,
    bg_mmol: /^(([2-9])|([1-2][0-9]))(\.[0-9])?$/,
    dexcom_server: /^(us|ous|jp)$/,
    ns_protocol: /^(http|https)$/,
    clock_timezone: /^.{2,}$/,
    time_format: /^(12|24)$/,
    email_format: /^[\w-\.]+(\+[A-Za-z0-9]+)?@([\w-]+\.)+[\w-]{2,4}$/,
    not_empty: /^.{1,}$/,
    custom_nodatatimer: /^(?:[6-9]|[1-5][0-9]|60)?$/,
    web_auth_password: /^.{8,64}$/,
};

// Mirror of the BG threshold ordering check added in data/script.js validateBG.
// Returns true iff urgent_low < low < high < urgent_high.
export function bgThresholdsOrdered(urgentLow, low, high, urgentHigh) {
    return urgentLow < low && low < high && high < urgentHigh;
}

// Mirror of the unit conversion in createJson — the form holds values in the
// user's chosen display unit; createJson always serializes as mg/dL.
export function toMgdl(rawValue, units) {
    if (units === 'mgdl') {
        return parseInt(rawValue, 10) || 0;
    }
    return Math.round((parseFloat(rawValue) || 0) * 18);
}
