// signaling/SignalingClient.cpp
// Stub de SignalingClient — structure + logs.
// À compléter avec IXWebSocket (recommandé) ou une lib WebSocket C++ NDK-compatible.
//
// Intégration IXWebSocket (TODO) :
//   1. Ajouter IXWebSocket comme sous-module CMake
//   2. Remplacer receiveLoop() par ix::WebSocket callbacks
//   3. sendSdp/sendIce appellent m_ws.send(json)

#include "SignalingClient.h"
#include "Util.h"

SignalingClient::SignalingClient(const std::string &url) : m_url(url) {
    LOGI("[SignalingClient] Initialisé — URL: %s", url.c_str());
}

SignalingClient::~SignalingClient() {
    disconnect();
}

void SignalingClient::connect() {
    if (m_running.exchange(true)) {
        LOGW("[SignalingClient] connect() déjà appelé");
        return;
    }
    LOGI("[SignalingClient] Connexion à %s ...", m_url.c_str());
    // TODO: ouvrir WebSocket vers m_url
    // m_ws.setUrl(m_url);
    // m_ws.start();
    m_recv_thread = std::thread(&SignalingClient::receiveLoop, this);
}

void SignalingClient::disconnect() {
    m_running = false;
    m_connected = false;
    // TODO: m_ws.stop();
    if (m_recv_thread.joinable()) m_recv_thread.join();
    LOGI("[SignalingClient] Déconnecté");
    if (onDisconnected) onDisconnected();
}

void SignalingClient::sendSdp(const SdpMessage &sdp) {
    if (!m_connected) {
        LOGW("[SignalingClient] sendSdp ignoré — non connecté");
        return;
    }
    // TODO: sérialiser en JSON et envoyer via WebSocket
    // std::string json = "{\"type\":\"" + sdp.type + "\",\"sdp\":\"" + sdp.sdp + "\"}";
    // m_ws.send(json);
    LOGI("[SignalingClient] sendSdp type=%s (stub)", sdp.type.c_str());
}

void SignalingClient::sendIce(const IceCandidate &candidate) {
    if (!m_connected) {
        LOGW("[SignalingClient] sendIce ignoré — non connecté");
        return;
    }
    // TODO: sérialiser et envoyer via WebSocket
    LOGI("[SignalingClient] sendIce mid=%s (stub)", candidate.sdpMid.c_str());
}

void SignalingClient::receiveLoop() {
    LOGI("[SignalingClient] receiveLoop démarré (stub)");
    // TODO: boucle de lecture WebSocket
    // while (m_running) {
    //     auto msg = m_ws.receive();
    //     dispatchMessage(msg.str);
    // }
    while (m_running.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    LOGI("[SignalingClient] receiveLoop arrêté");
}

void SignalingClient::dispatchMessage(const std::string &json) {
    // Parser le JSON et dispatcher sur les callbacks
    // TODO: utiliser nlohmann/json ou un parser léger
    LOGD("[SignalingClient] Message reçu: %s", json.c_str());
}
