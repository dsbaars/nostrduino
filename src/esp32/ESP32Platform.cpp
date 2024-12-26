#include "ESP32Platform.h"

namespace nostr {
namespace esp32 {
namespace ESP32Platform {

unsigned long getUnixTimestamp() {
    time_t now;
    struct tm timeinfo;
    while (!getLocalTime(&timeinfo)) {
        Serial.println("Still waiting for ntp sync");
        delay(10);
    }
    time(&now);
    return now;
}

long int getRealRandom(long int min, long int max) {
    uint32_t rand = esp_random();
    return (rand % (max - min + 1)) + min;
}

void serialLogger(const NostrString &str) {
    Serial.println(str.c_str());
}

void initWifi(NostrString ssid, NostrString passphrase, int unused) {
    esp_wifi_start();
    WiFi.begin(ssid, passphrase);
    Serial.println("Connecting to " + ssid);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

void initTime(const char *ntpServer, long unused1, int unused2) {
    configTime(0, 0, ntpServer);
}

void initNostr(bool withLogger) {
    bootloader_random_enable();
    Utils::init();
    nostr::Utils::setUnixTimeSecondsProvider(getUnixTimestamp);
    if (withLogger)
        nostr::Utils::setLogger(serialLogger);
    nostr::Utils::setRealRandom(getRealRandom);
}

void close() {
    Utils::close();
}

ESP32Transport *getTransport() {
    return new nostr::esp32::ESP32Transport();
}

} // namespace ESP32Platform
} // namespace esp32
} // namespace nostr 