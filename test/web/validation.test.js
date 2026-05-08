// Tests for the web UI validation patterns and small helpers.
// Run via: npm test (which invokes `vitest run`).

import { describe, expect, it } from 'vitest';
import { bgThresholdsOrdered, patterns, toMgdl } from './validation.js';

describe('regex patterns', () => {
    it('ssid accepts printable ASCII 1-32 chars', () => {
        expect(patterns.ssid.test('home')).toBe(true);
        expect(patterns.ssid.test('a'.repeat(32))).toBe(true);
        expect(patterns.ssid.test('')).toBe(false);
        expect(patterns.ssid.test('a'.repeat(33))).toBe(false);
        expect(patterns.ssid.test('hi\nthere')).toBe(false);
    });

    it('wifi_password requires at least 8 chars', () => {
        expect(patterns.wifi_password.test('1234567')).toBe(false);
        expect(patterns.wifi_password.test('12345678')).toBe(true);
    });

    it('dexcom_username requires at least 6 chars', () => {
        expect(patterns.dexcom_username.test('short')).toBe(false);
        expect(patterns.dexcom_username.test('parent@example.com')).toBe(true);
    });

    it('dexcom_password is 8-20 chars', () => {
        expect(patterns.dexcom_password.test('1234567')).toBe(false);
        expect(patterns.dexcom_password.test('12345678')).toBe(true);
        expect(patterns.dexcom_password.test('a'.repeat(20))).toBe(true);
        expect(patterns.dexcom_password.test('a'.repeat(21))).toBe(false);
    });

    it('api_secret accepts empty or 12+ chars', () => {
        expect(patterns.api_secret.test('')).toBe(true);
        expect(patterns.api_secret.test('short')).toBe(false);
        expect(patterns.api_secret.test('a'.repeat(12))).toBe(true);
    });

    it('bg_mgdl accepts physiological range 30-399', () => {
        expect(patterns.bg_mgdl.test('30')).toBe(true);
        expect(patterns.bg_mgdl.test('70')).toBe(true);
        expect(patterns.bg_mgdl.test('180')).toBe(true);
        expect(patterns.bg_mgdl.test('399')).toBe(true);
        expect(patterns.bg_mgdl.test('29')).toBe(false);
        expect(patterns.bg_mgdl.test('400')).toBe(false);
        expect(patterns.bg_mgdl.test('abc')).toBe(false);
    });

    it('bg_mmol accepts 2-29 with optional one decimal', () => {
        expect(patterns.bg_mmol.test('5.5')).toBe(true);
        expect(patterns.bg_mmol.test('10')).toBe(true);
        expect(patterns.bg_mmol.test('29')).toBe(true);
        expect(patterns.bg_mmol.test('1')).toBe(false);
        expect(patterns.bg_mmol.test('30')).toBe(false);
    });

    it('dexcom_server is one of us/ous/jp', () => {
        for (const v of ['us', 'ous', 'jp']) expect(patterns.dexcom_server.test(v)).toBe(true);
        expect(patterns.dexcom_server.test('eu')).toBe(false);
    });

    it('ns_hostname accepts FQDN and IPv4 dotted-quad', () => {
        expect(patterns.ns_hostname.test('mybloodsugar.example.com')).toBe(true);
        expect(patterns.ns_hostname.test('192.168.1.42')).toBe(true);
        expect(patterns.ns_hostname.test('not a host')).toBe(false);
    });

    it('time_format restricted to 12 or 24', () => {
        expect(patterns.time_format.test('12')).toBe(true);
        expect(patterns.time_format.test('24')).toBe(true);
        expect(patterns.time_format.test('48')).toBe(false);
    });

    it('email_format catches the obvious bad shapes', () => {
        expect(patterns.email_format.test('parent@example.com')).toBe(true);
        expect(patterns.email_format.test('parent+kid@sub.example.com')).toBe(true);
        expect(patterns.email_format.test('not-an-email')).toBe(false);
        expect(patterns.email_format.test('@example.com')).toBe(false);
    });

    it('web_auth_password is 8-64 chars', () => {
        expect(patterns.web_auth_password.test('1234567')).toBe(false);
        expect(patterns.web_auth_password.test('12345678')).toBe(true);
        expect(patterns.web_auth_password.test('a'.repeat(64))).toBe(true);
        expect(patterns.web_auth_password.test('a'.repeat(65))).toBe(false);
    });
});

describe('bgThresholdsOrdered', () => {
    it('accepts strictly ordered defaults', () => {
        expect(bgThresholdsOrdered(55, 70, 180, 250)).toBe(true);
    });

    it('rejects equal adjacent values', () => {
        expect(bgThresholdsOrdered(70, 70, 180, 250)).toBe(false);
        expect(bgThresholdsOrdered(55, 70, 180, 180)).toBe(false);
    });

    it('rejects inverted ordering', () => {
        expect(bgThresholdsOrdered(70, 55, 180, 250)).toBe(false);
        expect(bgThresholdsOrdered(55, 200, 180, 250)).toBe(false);
    });
});

describe('toMgdl unit conversion', () => {
    it('passes mgdl values through', () => {
        expect(toMgdl('100', 'mgdl')).toBe(100);
        expect(toMgdl('70', 'mgdl')).toBe(70);
    });

    it('multiplies mmol by 18 and rounds', () => {
        expect(toMgdl('5.5', 'mmol')).toBe(99);
        expect(toMgdl('10', 'mmol')).toBe(180);
    });

    it('returns 0 for empty/garbage input', () => {
        expect(toMgdl('', 'mgdl')).toBe(0);
        expect(toMgdl('abc', 'mgdl')).toBe(0);
        expect(toMgdl('', 'mmol')).toBe(0);
    });
});
