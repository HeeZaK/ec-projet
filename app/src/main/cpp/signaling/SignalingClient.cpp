// signaling/SignalingClient.cpp
// Implémentation WebSocket avec IXWebSocket.
// Si IXWebSocket n'est pas encore en submodule : compile en mode stub avec logs.

#include "SignalingClient.h"
#include "SdpSerializer.h"
#include "JsonProtocol.h"
#include "core/Logger.h"

// ── Inclusion conditionnelle IXWebSocket ─────────────────────────────────────
#if __has_include("IXWebSocket.h")
#  include "IXWebSocket.h"
#  include "IXNetSystem.h"
#  define HAS_IXWEBSOCKET 1
#else
#  define HAS_IXWEBSOCKET 0
#endif

#if HAS_IXWEBSOCKET
// Cast helper — évite le void* quand IXWebSocket est disponible
static ix::WebSocket *ws_ptr(void *h) { return static_cast<ix::WebSocket *>(h); }
#endif

SignalingClient::SignalingClient(const std::string &url) : m_url(url) {
    LOGI("[Signaling] URL: %s", url.c_str());
#if HAS_IXWEBSOCKET
    ix::initNetSystem();
    auto *ws = new ix::WebSocket();
    ws->setUrl(m_url);
    ws->setHandshakeTimeout(5);
    ws->setPingInterval(20);
    m_ws_handle = ws;
#endif
}

SignalingClient::~SignalingClient() {
    disconnect();
#if HAS_IXWEBSOCKET
    delete ws_ptr(m_ws_handle);
    m_ws_handle = nullptr;
    ix::uninitNetSystem();
#endif
}

void SignalingClient::connect() {
    if (m_running.exchange(true)) {
        LOGW("[Signaling] connect() déjà appelé");
        return;
    }
    LOGI("[Signaling] Connexion à %s", m_url.c_str());

#if HAS_IXWEBSOCKET
    ws_ptr(m_ws_handle)->setOnMessageCallback(
        [this](const ix::WebSocketMessagePtr &msg) {
            if (msg->type == ix::WebSocketMessageType::Open) {
                m_connected = true;
                LOGI("[Signaling] Connecté");
                if (onConnected) onConnected();

            } else if (msg->type == ix::WebSocketMessageType::Close) {
                m_connected = false;
                LOGW("[Signaling] Déconnecté (code=%d)", (int)msg->closeInfo.code);
                m_running = false;
                if (onDisconnected) onDisconnected();

            } else if (msg->type == ix::WebSocketMessageType::Message) {
                LOGD("[Signaling] Message reçu: %s", msg->str.c_str());
                dispatchMessage(msg->str);

            } else if (msg->type == ix::WebSocketMessageType::Error) {
                LOGE("[Signaling] Erreur WS: %s", msg->errorInfo.reason.c_str());
            }
        });
    ws_ptr(m_ws_handle)->start();
#else
    LOGW("[Signaling] IXWebSocket non disponible — mode stub");
    // Stub : simuler une connexion pour permettre les tests locaux
    m_connected = true;
    if (onConnected) onConnected();
#endif
}

void SignalingClient::disconnect() {
    if (!m_running.exchange(false)) return;
#if HAS_IXWEBSOCKET
    ws_ptr(m_ws_handle)->stop();
#endif
    m_connected = false;
    LOGI("[Signaling] Déconnecté");
    if (onDisconnected) onDisconnected();
}

void SignalingClient::send(const std::string &json) {
    if (!m_connected) {
        LOGW("[Signaling] send() ignoré — non connecté");
        return;
    }
    std::lock_guard<std::mutex> lock(m_send_mutex);
#if HAS_IXWEBSOCKET
    ws_ptr(m_ws_handle)->send(json);
#else
    LOGD("[Signaling] send() stub: %s", json.c_str());
#endif
}

void SignalingClient::sendSdp(const SdpMessage &sdp) {
    std::string json = JsonProtocol::encodeSdp(sdp);
    LOGI("[Signaling] sendSdp type=%s", sdp.type.c_str());
    send(json);
}

void SignalingClient::sendIce(const IceCandidate &candidate) {
    std::string json = JsonProtocol::encodeIce(candidate);
    LOGD("[Signaling] sendIce mid=%s", candidate.sdpMid.c_str());
    send(json);
}

void SignalingClient::dispatchMessage(const std::string &json) {
    std::string type = JsonProtocol::extractType(json);

    if (type == "answer") {
        SdpMessage sdp = JsonProtocol::decodeSdp(json);
        LOGI("[Signaling] SDP answer reçu");
        if (onRemoteSdp) onRemoteSdp(sdp);

    } else if (type == "candidate") {
        IceCandidate ice = JsonProtocol::decodeIce(json);
        LOGD("[Signaling] ICE candidate reçu mid=%s", ice.sdpMid.c_str());
        if (onRemoteIce) onRemoteIce(ice);

    } else {
        LOGW("[Signaling] Message inconnu type='%s'", type.c_str());
    }
}
