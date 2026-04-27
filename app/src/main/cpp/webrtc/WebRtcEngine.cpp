// webrtc/WebRtcEngine.cpp
// Stub compilable — à remplir une fois libwebrtc linké dans CMakeLists.

#include "WebRtcEngine.h"
#include "Util.h"
#include "core/AppConfig.h"
#include "core/DeviceRuntime.h"

WebRtcEngine::WebRtcEngine() {
    LOGI("[WebRtcEngine] Construit");
}

WebRtcEngine::~WebRtcEngine() {
    destroy();
}

bool WebRtcEngine::init() {
    if (m_initialized) {
        LOGW("[WebRtcEngine] init() appelé plusieurs fois");
        return true;
    }

    LOGI("[WebRtcEngine] Initialisation...");

    // ── Signaling ──────────────────────────────────────────────────────────
    m_signaling = std::make_unique<SignalingClient>(AppConfig::SIGNALING_URL);

    m_signaling->onRemoteSdp = [this](const SdpMessage &sdp) {
        LOGI("[WebRtcEngine] SDP distant reçu: %s", sdp.type.c_str());
        setRemoteSdp(sdp);
    };
    m_signaling->onRemoteIce = [this](const IceCandidate &c) {
        LOGI("[WebRtcEngine] ICE distant reçu: %s", c.sdpMid.c_str());
        addIceCandidate(c);
    };
    m_signaling->onConnected = [this]() {
        LOGI("[WebRtcEngine] Signaling connecté — création offer");
        DeviceRuntime::instance().setConnected(true);
        createOffer();
    };
    m_signaling->onDisconnected = [this]() {
        LOGW("[WebRtcEngine] Signaling déconnecté");
        DeviceRuntime::instance().setConnected(false);
        if (onConnectionStateChange) onConnectionStateChange(false);
    };

    // ── Telemetry ──────────────────────────────────────────────────────────
    // La sendFn sera remplacée par DataChannel::send() une fois la PC créée.
    m_telemetry = std::make_unique<TelemetryPublisher>(
        [](const std::string &payload) {
            LOGD("[WebRtcEngine] Telemetry (stub send): %s", payload.c_str());
            // TODO: m_data_channel->Send(payload)
        }
    );

    // ── PeerConnectionFactory ──────────────────────────────────────────────
    // TODO: initialiser libwebrtc
    // rtc::InitializeSSL();
    // m_pcf = webrtc::CreatePeerConnectionFactory(...);
    // ASSERT(m_pcf, "[WebRtcEngine] PeerConnectionFactory init failed");

    m_initialized = true;
    LOGI("[WebRtcEngine] Initialisé (stub — libwebrtc à linker)");
    return true;
}

void WebRtcEngine::destroy() {
    if (!m_initialized) return;
    if (m_telemetry)  m_telemetry->stop();
    if (m_signaling)  m_signaling->disconnect();
    // TODO: m_pc = nullptr; m_pcf = nullptr; rtc::CleanupSSL();
    m_initialized = false;
    LOGI("[WebRtcEngine] Détruit");
}

void WebRtcEngine::createOffer() {
    LOGI("[WebRtcEngine] createOffer() (stub)");
    // TODO:
    // auto obs = rtc::make_ref_counted<CreateSdpObserver>(...);
    // m_pc->CreateOffer(obs.get(), webrtc::PeerConnectionInterface::RTCOfferAnswerOptions{});
}

void WebRtcEngine::setRemoteSdp(const SdpMessage &sdp) {
    LOGI("[WebRtcEngine] setRemoteSdp type=%s (stub)", sdp.type.c_str());
    // TODO: m_pc->SetRemoteDescription(...)
}

void WebRtcEngine::addIceCandidate(const IceCandidate &candidate) {
    LOGI("[WebRtcEngine] addIceCandidate mid=%s (stub)", candidate.sdpMid.c_str());
    // TODO: m_pc->AddIceCandidate(...)
}
