
#include "esp32/ESP32Platform.h"

#include <Arduino.h>

#include "NostrTransport.h"
#include "services/NWC.h"

// CONFIGURATION
#define WIFI_SSID "Bulgur"
#define WIFI_PASS "panged1barmie"
// IMPORTANT !!! Set a valid NWC url here
#define NWC_URL                                                                                                                                                                                        \
    "nostr+walletconnect://7c3971e292f4c5c35211707672b98a1a56ee6baa4a10bb2c0569b56e0380dd79?relay=wss://relay.getalby.com/v1&secret=59c7c9b2b99cd49554a2cb849e49472985c06d2817830727366d430d113174b5&lud16=zestywave833927@getalby.com"
// Note: running this setting with default values will send 10 sats to the address below
#define PAYOUT_ADDRESS "silverowl2@primal.net"
#define PAYOUT_AMOUNT_MSAT 10000

void testNWC();

void setup() {
    ////////////////////////
    /// INITIALIZATION
    ///   Note: you need some form of this code in your sketch
    ///         or the library will not work properly.
    ///         If you don't know what to do, just copy this code.
    ///         If you are initializing time and wifi somewhere else in your
    ///         code, you can just omit the related lines here and call only
    ///         initNostr
    ////////////////////////
    Serial.begin(115200);

    Serial.println("Init wifi");
    nostr::esp32::ESP32Platform::initWifi(WIFI_SSID, WIFI_PASS);

    Serial.println("Init time");
    nostr::esp32::ESP32Platform::initTime("192.168.20.108");

    Serial.println("Init Nostr");
    nostr::esp32::ESP32Platform::initNostr(true);

    Serial.println("Ready!");

    ////////////////////////
    /// END OF INITIALIZATION
    ////////////////////////

    testNWC();
}

nostr::NWC *nwc;
nostr::Transport *transport;

void loop() {
    nwc->loop();
}

void testNWC() {
    try {
        transport = nostr::esp32::ESP32Platform::getTransport();
        nwc = new nostr::NWC(transport, NWC_URL);

        // nwc->getBalance([&](nostr::GetBalanceResponse resp) { Serial.println("[!] Balance: " + String(resp.balance) + " msatoshis"); },
        //                 [](String err, String errMsg) { Serial.println("[!] Error: " + err + " " + errMsg); });

        // nwc->getInfo(
        //     [&](nostr::GetInfoResponse resp) {
        //         Serial.println("[!] Alias: " + resp.alias);
        //         Serial.println("[!] Color: " + resp.color);
        //         Serial.println("[!] Pubkey: " + resp.pubkey);
        //         Serial.println("[!] Network: " + resp.network);
        //         Serial.println("[!] Block height: " + String(resp.blockHeight));
        //         Serial.println("[!] Block hash: " + resp.blockHash);
        //         Serial.println("[!] Methods: ");
        //         for (auto method : resp.methods) {
        //             Serial.println("  " + method);
        //         }
        //     },
        //     [](String err, String errMsg) { Serial.println("[!] Error: " + err + " " + errMsg); });

        //  transport->getInvoiceFromLNAddr(PAYOUT_ADDRESS, PAYOUT_AMOUNT_MSAT, "Arduino NWC test", [&](String invoice) {
        //     if (NostrString_equals(invoice, "")) {
        //         Serial.println("[!] Error: Could not get invoice from LN address");
        //     }
        //     Serial.println("[!] Paying " + String(PAYOUT_AMOUNT_MSAT) + " msats to " + PAYOUT_ADDRESS + " invoice: " + invoice);
        //     nwc->payInvoice(
        //         invoice, PAYOUT_AMOUNT_MSAT, [&](nostr::PayInvoiceResponse resp) { Serial.println("[!] Payment successful"); },
        //         [](String err, String errMsg) { Serial.println("[!] Error: " + err + " " + errMsg); });
        // });

        nwc->subscribeNotifications();

    } catch (std::exception &e) {
        Serial.println("[!] Exception: " + String(e.what()));
    }
}