#ifndef _NOSTR_ESP32_PLATFORM_H
#define _NOSTR_ESP32_PLATFORM_H 1
#include "NostrCommon.h"

#ifdef _ESP32_BOARD_
#include "ESP32Transport.h"
#include "NostrString.h"
#include "NostrUtils.h"
#include "WiFi.h"
#include "bootloader_random.h"
#include "esp_random.h"
#include "esp_wifi.h"
#include "time.h"
#include <initializer_list>
#include <vector>
namespace nostr {
namespace esp32 {
namespace ESP32Platform {

unsigned long getUnixTimestamp();
long int getRealRandom(long int min, long int max);
void serialLogger(const NostrString &str);
void initWifi(NostrString ssid, NostrString passphrase, int unused = 6);
void initTime(const char *ntpServer, long unused1 = 0, int unused2 = 3600);
void initNostr(bool withLogger);
void close();
ESP32Transport *getTransport();

} // namespace ESP32Platform
} // namespace esp32
} // namespace nostr
#endif
#endif