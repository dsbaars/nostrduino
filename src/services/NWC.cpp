#include "NWC.h"

#include "Nip47.h"
using namespace nostr;

NWC::~NWC()
{
    this->close();
}

NWC::NWC(Transport *transport, NostrString nwcUrl)
{
    this->pool = std::unique_ptr<NostrPool>(new NostrPool(transport));
    Nip47::parseNWC(nwcUrl, this->nwc);

    Utils::log("NWC RELAY: " + this->nwc.relay);
    Utils::log("NWC PUBKEY: " + this->nwc.pubkey);
    Utils::log("NWC PRIVKEY: ******");

    NostrString accountSecret = this->nwc.secret;

    this->nip47 = Nip47(Nip04(), this->nwc.pubkey, accountSecret);
    this->accountPubKey = Utils::getPublicKey(accountSecret);
}

void NWC::close()
{
    this->pool->close();
    this->callbacks.clear();
}

void NWC::loop()
{
    this->pool->loop();
    for (auto it = this->callbacks.begin(); it != this->callbacks.end();)
    {
        if (it->get()->n == 0)
        {
            NostrString subId = it->get()->subId;
            this->pool->closeSubscription(subId);
            it = this->callbacks.erase(it);
            continue;
        }
        if (Utils::unixTimeSeconds() - it->get()->timestampSeconds > 60 * 10)
        {
            NostrString subId = it->get()->subId;
            this->pool->closeSubscription(subId);
            it->get()->onErr("OTHER", "timeout");
            it = this->callbacks.erase(it);
        }
        else
        {
            it++;
        }
    }
}

NostrString NWC::sendEvent(SignedNostrEvent *event)
{
    NostrString subId = this->pool->subscribeMany(
        {this->nwc.relay}, {{{"kinds", {"23195", "13194"}}, {"#p", {this->accountPubKey}}, {"#e", {event->getId()}}}},
        [&](const String &subId, nostr::SignedNostrEvent *event)
        {
            NostrString eventRef = event->getTags()->getTag("e")[0];
            for (auto it = this->callbacks.begin(); it != this->callbacks.end(); it++)
            {
                if (NostrString_equals(it->get()->eventId, eventRef))
                {
                    if (it->get()->n > 0)
                    {
                        it->get()->call(&this->nip47, event);
                    }
                    it->get()->n--;
                    break;
                }
            }
        },
        [&](const String &subId, const String &reason)
        { Utils::log("NWC: closed subscription: " + reason); }, [&](const String &subId)
        { Utils::log("NWC: EOS"); });
    this->pool->publish({this->nwc.relay}, event, [&](const NostrString &eventId, bool status, const NostrString &msg)
                        {
        if (!status) {
            Utils::log("NWC: error sending event: " + msg);
        } else {
            Utils::log("NWC: event sent: " + eventId);
        } });
    return subId;
}

void NWC::payInvoice(NostrString invoice, unsigned long amount, std::function<void(PayInvoiceResponse)> onRes, std::function<void(NostrString, NostrString)> onErr)
{
    SignedNostrEvent ev = this->nip47.payInvoice(invoice, amount);
    std::unique_ptr<NWCResponseCallback<PayInvoiceResponse>> callback(new NWCResponseCallback<PayInvoiceResponse>());
    callback->onRes = onRes;
    callback->onErr = onErr;
    callback->timestampSeconds = Utils::unixTimeSeconds();
    callback->eventId = ev.getId();
    callback->n = 1;
    callback->subId = this->sendEvent(&ev);
    this->callbacks.push_back(std::move(callback));
}

void NWC::multiPayInvoice(std::initializer_list<Invoice> invoices, std::function<void(MultiPayInvoiceResponse)> onRes, std::function<void(NostrString, NostrString)> onErr)
{
    SignedNostrEvent ev = this->nip47.multiPayInvoice(invoices);
    std::unique_ptr<NWCResponseCallback<MultiPayInvoiceResponse>> callback(new NWCResponseCallback<MultiPayInvoiceResponse>());
    callback->onRes = onRes;
    callback->onErr = onErr;
    callback->timestampSeconds = Utils::unixTimeSeconds();
    callback->eventId = ev.getId();
    callback->n = invoices.size();
    callback->subId = this->sendEvent(&ev);
    this->callbacks.push_back(std::move(callback));
}

void NWC::payKeySend(NostrString pubkey, unsigned long amount, NostrString preimage, std::initializer_list<TLVRecords> tlv, std::function<void(PayKeySendResponse)> onRes,
                     std::function<void(NostrString, NostrString)> onErr)
{
    SignedNostrEvent ev = this->nip47.payKeySend(pubkey, amount, preimage, tlv);
    std::unique_ptr<NWCResponseCallback<PayKeySendResponse>> callback(new NWCResponseCallback<PayKeySendResponse>());
    callback->onRes = onRes;
    callback->onErr = onErr;
    callback->timestampSeconds = Utils::unixTimeSeconds();
    callback->eventId = ev.getId();
    callback->n = 1;
    callback->subId = this->sendEvent(&ev);
    this->callbacks.push_back(std::move(callback));
}

void NWC::multiPayKeySend(std::initializer_list<KeySend> keySends, std::function<void(MultiPayKeySendResponse)> onRes, std::function<void(NostrString, NostrString)> onErr)
{
    SignedNostrEvent ev = this->nip47.multiPayKeySend(keySends);
    std::unique_ptr<NWCResponseCallback<MultiPayKeySendResponse>> callback(new NWCResponseCallback<MultiPayKeySendResponse>());
    callback->onRes = onRes;
    callback->onErr = onErr;
    callback->timestampSeconds = Utils::unixTimeSeconds();
    callback->eventId = ev.getId();
    callback->n = keySends.size();
    callback->subId = this->sendEvent(&ev);
    this->callbacks.push_back(std::move(callback));
}

void NWC::makeInvoice(unsigned long amount, NostrString description, NostrString descriptionHash, unsigned long expiry, std::function<void(MakeInvoiceResponse)> onRes,
                      std::function<void(NostrString, NostrString)> onErr)
{
    SignedNostrEvent ev = this->nip47.makeInvoice(amount, description, descriptionHash, expiry);
    std::unique_ptr<NWCResponseCallback<MakeInvoiceResponse>> callback(new NWCResponseCallback<MakeInvoiceResponse>());
    callback->onRes = onRes;
    callback->onErr = onErr;
    callback->timestampSeconds = Utils::unixTimeSeconds();
    callback->eventId = ev.getId();
    callback->n = 1;
    callback->subId = this->sendEvent(&ev);
    this->callbacks.push_back(std::move(callback));
}

void NWC::lookUpPaymentHash(NostrString paymentHash, std::function<void(LookUpInvoiceResponse)> onRes, std::function<void(NostrString, NostrString)> onErr)
{
    SignedNostrEvent ev = this->nip47.lookUpPaymentHash(paymentHash);
    std::unique_ptr<NWCResponseCallback<LookUpInvoiceResponse>> callback(new NWCResponseCallback<LookUpInvoiceResponse>());
    callback->onRes = onRes;
    callback->onErr = onErr;
    callback->timestampSeconds = Utils::unixTimeSeconds();
    callback->eventId = ev.getId();
    callback->n = 1;
    callback->subId = this->sendEvent(&ev);
    this->callbacks.push_back(std::move(callback));
}

void NWC::lookUpInvoice(NostrString invoice, std::function<void(LookUpInvoiceResponse)> onRes, std::function<void(NostrString, NostrString)> onErr)
{
    SignedNostrEvent ev = this->nip47.lookUpInvoice(invoice);
    std::unique_ptr<NWCResponseCallback<LookUpInvoiceResponse>> callback(new NWCResponseCallback<LookUpInvoiceResponse>());
    callback->onRes = onRes;
    callback->onErr = onErr;
    callback->timestampSeconds = Utils::unixTimeSeconds();
    callback->eventId = ev.getId();
    callback->n = 1;
    callback->subId = this->sendEvent(&ev);
    this->callbacks.push_back(std::move(callback));
}

void NWC::listTransactions(unsigned long from, unsigned long until, int limit, int offset, bool unpaid, NostrString type, std::function<void(ListTransactionsResponse)> onRes,
                           std::function<void(NostrString, NostrString)> onErr)
{
    SignedNostrEvent ev = this->nip47.listTransactions(from, until);
    std::unique_ptr<NWCResponseCallback<ListTransactionsResponse>> callback(new NWCResponseCallback<ListTransactionsResponse>());
    callback->onRes = onRes;
    callback->onErr = onErr;
    callback->timestampSeconds = Utils::unixTimeSeconds();
    callback->eventId = ev.getId();
    callback->n = 1;
    callback->subId = this->sendEvent(&ev);
    this->callbacks.push_back(std::move(callback));
}

void NWC::getBalance(std::function<void(GetBalanceResponse)> onRes, std::function<void(NostrString, NostrString)> onErr)
{
    SignedNostrEvent ev = this->nip47.getBalance();
    std::unique_ptr<NWCResponseCallback<GetBalanceResponse>> callback(new NWCResponseCallback<GetBalanceResponse>());
    callback->onRes = onRes;
    callback->onErr = onErr;
    callback->timestampSeconds = Utils::unixTimeSeconds();
    callback->eventId = ev.getId();
    callback->n = 1;
    callback->subId = this->sendEvent(&ev);
    this->callbacks.push_back(std::move(callback));
}

void NWC::subscribeNotifications(std::function<void(SubscribeNotificationsResponse)> onRes, std::function<void(NostrString, NostrString)> onErr)
{
    String subId = this->pool->subscribeMany({this->nwc.relay}, {{{"kinds", {"13194", "23196"}}, {"authors", {this->nwc.pubkey}}}

                                                                },
                                             [&](const String &subId, nostr::SignedNostrEvent *event)
                                             {
                                                 if (event->getKind() == 23196)
                                                 {
                                                     NostrString content = this->nip47.nip04.decrypt(this->nwc.secret, this->nwc.pubkey, event->getContent());
                                                     // Nip47::parseResponse(event, )
                                                     JsonDocument doc;
                                                     Utils::jsonParse(&content, &doc);
                                                     String json;
                                                     serializeJson(doc, json);
                                                     Serial.println("NWC Sub Decoded: " + json);
                                                 }

                                                 JsonDocument doc;
                                                 JsonArray arr = doc["data"].to<JsonArray>();
                                                 event->toSendableEvent(arr);
                                                 String json;
                                                 serializeJson(arr, json);
                                                 Serial.println("NWC Event received: " + json);
                                             },
                                             [&](const String &subId, const String &reason)
                                             { Serial.println("NWC Subscription closed: " + reason); }, [&](const String &subId)
                                             { Serial.println("NWC Subscription EOSE: " + subId); });

        std::vector<nostr::NostrRelay *> *relays = this->pool->getConnectedRelays();
        for (nostr::NostrRelay *relay : *relays)
        {
            Serial.println("[ Nostr ] Registering to connection events of: " + relay->getUrl());
            relay->getConnection()->addConnectionStatusListener([&](const nostr::ConnectionStatus &status)
                                                                { 
                String sstatus="UNKNOWN";
                if(status==nostr::ConnectionStatus::CONNECTED){
                    // nostrIsConnected = true;
                    sstatus="CONNECTED";
                }else if(status==nostr::ConnectionStatus::DISCONNECTED){
                    // nostrIsConnected = false;
                    // nostrIsSubscribed = false;
                     sstatus="DISCONNECTED";
                }else if(status==nostr::ConnectionStatus::ERROR){
                    sstatus = "ERROR";
                }
                Serial.println("[ Nostr ] Connection status changed: " + sstatus); 
                });
        }
}

void NWC::getInfo(std::function<void(GetInfoResponse)> onRes, std::function<void(NostrString, NostrString)> onErr)
{
    SignedNostrEvent ev = this->nip47.getInfo();
    std::unique_ptr<NWCResponseCallback<GetInfoResponse>> callback(new NWCResponseCallback<GetInfoResponse>());
    callback->onRes = onRes;
    callback->onErr = onErr;
    callback->timestampSeconds = Utils::unixTimeSeconds();
    callback->eventId = ev.getId();
    callback->n = 1;
    callback->subId = this->sendEvent(&ev);
    this->callbacks.push_back(std::move(callback));
}