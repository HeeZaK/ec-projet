// webrtc/DataChannelController.h
// Gestion du DataChannel WebRTC pour la télémétrie (GPS/batterie/commandes).

#pragma once

#include <string>
#include <functional>
#include <atomic>

#ifdef WEBRTC_ANDROID
#include "api/data_channel_interface.h"
#include "rtc_base/ref_counted_object.h"
#endif

// Callback commandes distantes → application
using RemoteCommandCallback = std::function<void(const std::string &command)>;

#ifdef WEBRTC_ANDROID

class DataChannelController
    : public rtc::RefCountedObject<webrtc::DataChannelObserver> {
public:
    explicit DataChannelController(
        rtc::scoped_refptr<webrtc::DataChannelInterface> dc);
    ~DataChannelController() override;

    // Envoie un payload telemetrie JSON
    void send(const std::string &payload);

    bool isOpen() const { return m_open.load(); }

    // Callback pour les commandes reçues du serveur distant
    RemoteCommandCallback onRemoteCommand;

    // webrtc::DataChannelObserver
    void OnStateChange()  override;
    void OnMessage(const webrtc::DataBuffer &buffer) override;
    void OnBufferedAmountChange(uint64_t /*sentDataSize*/) override {}

private:
    rtc::scoped_refptr<webrtc::DataChannelInterface> m_dc;
    std::atomic<bool> m_open{false};
};

#else // Stub

class DataChannelController {
public:
    void send(const std::string &payload);
    bool isOpen() const { return false; }
    RemoteCommandCallback onRemoteCommand;
};

#endif
