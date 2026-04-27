// signaling/SignalingClient.h
// Client de signaling WebRTC (SDP offer/answer + ICE candidates).
// Interface prête pour une implémentation WebSocket (IXWebSocket ou libcurl).
// Pour l'instant : stub compilable avec callbacks à brancher.

#pragma once

#include <string>
#include <functional>
#include <atomic>
#include <thread>

struct SdpMessage {
    std::string type;  // "offer" | "answer"
    std::string sdp;
};

struct IceCandidate {
    std::string candidate;
    std::string sdpMid;
    int         sdpMLineIndex = 0;
};

class SignalingClient {
public:
    // Callbacks branchés depuis WebRtcEngine
    std::function<void(const SdpMessage &)>   onRemoteSdp;
    std::function<void(const IceCandidate &)> onRemoteIce;
    std::function<void()>                     onConnected;
    std::function<void()>                     onDisconnected;

    explicit SignalingClient(const std::string &url);
    ~SignalingClient();

    // Connexion au serveur de signaling
    void connect();
    void disconnect();

    // Envoi vers le serveur
    void sendSdp(const SdpMessage &sdp);
    void sendIce(const IceCandidate &candidate);

    bool isConnected() const { return m_connected.load(); }

private:
    void receiveLoop();
    void dispatchMessage(const std::string &json);

    std::string        m_url;
    std::atomic<bool>  m_connected{false};
    std::atomic<bool>  m_running{false};
    std::thread        m_recv_thread;

    // TODO: remplacer par IXWebSocket::WebSocket ou libcurl handle
    // void *m_ws_handle = nullptr;
};
