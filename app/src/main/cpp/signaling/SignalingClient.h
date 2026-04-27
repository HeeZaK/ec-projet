// signaling/SignalingClient.h
// Client WebSocket de signaling WebRTC.
// Implémentation avec IXWebSocket (sous-module third_party/IXWebSocket).
// Protocole JSON : { type, sdp } pour SDP, { type, candidate, sdpMid, sdpMLineIndex } pour ICE.

#pragma once

#include <string>
#include <functional>
#include <atomic>
#include <mutex>

#ifdef IXWEBSOCKET_AVAILABLE
// Désactivé au préprocesseur si IXWebSocket n'est pas encore ajouté en submodule.
// La compilation reste possible en mode stub.
#endif

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

    void connect();
    void disconnect();

    void sendSdp(const SdpMessage &sdp);
    void sendIce(const IceCandidate &candidate);

    bool isConnected() const { return m_connected.load(); }

private:
    void dispatchMessage(const std::string &json);
    void send(const std::string &json);

    std::string       m_url;
    std::atomic<bool> m_connected{false};
    std::atomic<bool> m_running{false};
    std::mutex        m_send_mutex;

    // Handle IXWebSocket — void* pour compiler sans IXWebSocket headers
    // Remplacé par ix::WebSocket m_ws quand IXWebSocket est disponible
    void *m_ws_handle = nullptr;
};
