// webrtc/WebRtcEngine.cpp
// Implémentation complète WebRTC : PeerConnection, VideoTrack, AudioTrack, DataChannel.
// Compilation conditionnelle : fonctionnel avec libwebrtc, stub sans.

#include "WebRtcEngine.h"
#include "IceConfig.h"
#include "core/Logger.h"
#include "core/AppConfig.h"
#include "core/DeviceRuntime.h"

#ifdef WEBRTC_ANDROID
#include "api/create_peerconnection_factory.h"
#include "api/video_track_source_proxy.h"
#include "api/rtc_event_log/rtc_event_log_factory.h"
#include "api/task_queue/default_task_queue_factory.h"
#include "modules/audio_device/include/audio_device.h"
#include "modules/audio_processing/include/audio_processing.h"

// ── Observers SDP ────────────────────────────────────────────────────────────

class CreateSdpObserver
    : public rtc::RefCountedObject<webrtc::CreateSessionDescriptionObserver> {
public:
    using OnSuccessCb = std::function<void(webrtc::SessionDescriptionInterface *)>;
    using OnFailureCb = std::function<void(webrtc::RTCError)>;

    CreateSdpObserver(OnSuccessCb s, OnFailureCb f)
        : on_success_(std::move(s)), on_failure_(std::move(f)) {}

    void OnSuccess(webrtc::SessionDescriptionInterface *desc) override {
        if (on_success_) on_success_(desc);
    }
    void OnFailure(webrtc::RTCError error) override {
        LOGE("[CreateSdpObserver] Erreur: %s", error.message());
        if (on_failure_) on_failure_(error);
    }
private:
    OnSuccessCb on_success_;
    OnFailureCb on_failure_;
};

class SetSdpObserver
    : public rtc::RefCountedObject<webrtc::SetSessionDescriptionObserver> {
public:
    void OnSuccess()                  override { LOGD("[SetSdpObserver] OK"); }
    void OnFailure(webrtc::RTCError e) override {
        LOGE("[SetSdpObserver] Erreur: %s", e.message());
    }
};

#endif // WEBRTC_ANDROID

// ─── Constructor / Destructor ────────────────────────────────────────────────

WebRtcEngine::WebRtcEngine() {
    LOGI("[WebRtcEngine] Instancié");
}

WebRtcEngine::~WebRtcEngine() {
    destroy();
}

// ─── init() ──────────────────────────────────────────────────────────────────

bool WebRtcEngine::init() {
    if (m_initialized) {
        LOGW("[WebRtcEngine] init() déjà appelé");
        return true;
    }

    LOGI("[WebRtcEngine] Initialisation...");

    // ── Audio ────────────────────────────────────────────────────────────────
    m_audio = std::make_unique<AudioController>();
    m_audio->start();

    // ── VideoTrackSource ─────────────────────────────────────────────────────
    m_video_source = std::make_unique<VideoTrackSource>();

    // ── Signaling ────────────────────────────────────────────────────────────
    m_signaling = std::make_unique<SignalingClient>(AppConfig::SIGNALING_URL);

    m_signaling->onRemoteSdp = [this](const SdpMessage &sdp) {
        LOGI("[WebRtcEngine] SDP distant reçu type=%s", sdp.type.c_str());
        setRemoteSdp(sdp);
    };
    m_signaling->onRemoteIce = [this](const IceCandidate &c) {
        LOGD("[WebRtcEngine] ICE distant mid=%s", c.sdpMid.c_str());
        addIceCandidate(c);
    };
    m_signaling->onConnected = [this]() {
        LOGI("[WebRtcEngine] Signaling connecté → createOffer()");
        DeviceRuntime::instance().setConnected(true);
        createOffer();
    };
    m_signaling->onDisconnected = [this]() {
        LOGW("[WebRtcEngine] Signaling déconnecté");
        DeviceRuntime::instance().setConnected(false);
        if (onConnectionStateChange) onConnectionStateChange(false);
    };

    // ── Telemetry ────────────────────────────────────────────────────────────
    m_telemetry = std::make_unique<TelemetryPublisher>(
        [this](const std::string &payload) {
#ifdef WEBRTC_ANDROID
            if (m_data_channel && m_data_channel->isOpen())
                m_data_channel->send(payload);
#else
            LOGD("[WebRtcEngine] Telemetry (stub): %s", payload.c_str());
#endif
        });

#ifdef WEBRTC_ANDROID
    // ── Threads WebRTC ───────────────────────────────────────────────────────
    m_network_thread   = rtc::Thread::CreateWithSocketServer();
    m_worker_thread    = rtc::Thread::Create();
    m_signaling_thread = rtc::Thread::Create();
    m_network_thread->Start();
    m_worker_thread->Start();
    m_signaling_thread->Start();

    // ── PeerConnectionFactory ────────────────────────────────────────────────
    rtc::InitializeSSL();
    m_pcf = webrtc::CreatePeerConnectionFactory(
        m_network_thread.get(),
        m_worker_thread.get(),
        m_signaling_thread.get(),
        nullptr,                                    // ADM : null = default Android
        webrtc::CreateBuiltinAudioEncoderFactory(),
        webrtc::CreateBuiltinAudioDecoderFactory(),
        webrtc::CreateBuiltinVideoEncoderFactory(),
        webrtc::CreateBuiltinVideoDecoderFactory(),
        nullptr,                                    // audio mixer
        nullptr                                     // audio processing
    );
    if (!m_pcf) {
        LOGE("[WebRtcEngine] PeerConnectionFactory init FAILED");
        return false;
    }
    LOGI("[WebRtcEngine] PeerConnectionFactory OK");
#else
    LOGW("[WebRtcEngine] libwebrtc absent — mode stub");
#endif

    // ── Connexion Signaling ──────────────────────────────────────────────────
    m_signaling->connect();

    m_initialized = true;
    LOGI("[WebRtcEngine] Initialisé");
    return true;
}

// ─── destroy() ───────────────────────────────────────────────────────────────

void WebRtcEngine::destroy() {
    if (!m_initialized) return;

    LOGI("[WebRtcEngine] Destruction...");

    if (m_telemetry)  m_telemetry->stop();
    if (m_audio)      m_audio->stop();
    if (m_signaling)  m_signaling->disconnect();

#ifdef WEBRTC_ANDROID
    m_data_channel.reset();
    m_pc.release();
    m_pcf.release();
    rtc::CleanupSSL();
    if (m_signaling_thread) m_signaling_thread->Stop();
    if (m_worker_thread)    m_worker_thread->Stop();
    if (m_network_thread)   m_network_thread->Stop();
#endif

    m_initialized = false;
    LOGI("[WebRtcEngine] Détruit");
}

// ─── pushVideoFrame() ────────────────────────────────────────────────────────

void WebRtcEngine::pushVideoFrame(
        const uint8_t *rgba, int width, int height, int64_t timestampUs) {
    if (!m_video_source) return;
    RawVideoFrame f;
    f.data_rgba    = rgba;
    f.width        = width;
    f.height       = height;
    f.timestamp_us = timestampUs;
    m_video_source->OnFrameAvailable(f);
}

// ─── createOffer() ───────────────────────────────────────────────────────────

void WebRtcEngine::createOffer() {
    LOGI("[WebRtcEngine] createOffer()");

#ifdef WEBRTC_ANDROID
    if (!m_pcf) { LOGE("[WebRtcEngine] PCF null"); return; }

    // Observer PeerConnection
    m_peer_observer = std::make_unique<PeerObserver>();
    m_peer_observer->onLocalIce = [this](const IceCandidate &ic) {
        if (m_signaling) m_signaling->sendIce(ic);
    };
    m_peer_observer->onConnectionChange = [this](bool connected) {
        if (!connected && onConnectionStateChange) onConnectionStateChange(false);
        if (connected) {
            if (m_telemetry) m_telemetry->start(1000);
            if (onConnectionStateChange) onConnectionStateChange(true);
        }
    };

    // Créer la PeerConnection
    webrtc::PeerConnectionInterface::RTCConfiguration config = IceConfig::getConfig();
    webrtc::PeerConnectionDependencies deps(m_peer_observer.get());
    auto pc_or_error = m_pcf->CreatePeerConnectionOrError(config, std::move(deps));
    if (!pc_or_error.ok()) {
        LOGE("[WebRtcEngine] CreatePeerConnection failed: %s",
             pc_or_error.error().message());
        return;
    }
    m_pc = pc_or_error.MoveValue();

    // ── DataChannel telemetrie ───────────────────────────────────────────────
    webrtc::DataChannelInit dc_config;
    dc_config.ordered   = false;
    dc_config.maxRetransmits = 0; // UDP-like
    auto dc_or_error = m_pc->CreateDataChannelOrError("telemetry", &dc_config);
    if (dc_or_error.ok()) {
        m_data_channel = std::make_unique<DataChannelController>(
            dc_or_error.MoveValue());
        m_data_channel->onRemoteCommand = [](const std::string &cmd) {
            LOGI("[WebRtcEngine] Commande distante reçue: %s", cmd.c_str());
            // TODO: dispatcher vers l'application (ex: changer caméra, ajuster bitrate)
        };
    } else {
        LOGE("[WebRtcEngine] CreateDataChannel failed");
    }

    // ── VideoTrack ───────────────────────────────────────────────────────────
    auto video_source_proxy = webrtc::VideoTrackSourceProxy::Create(
        m_signaling_thread.get(), m_worker_thread.get(), m_video_source.get());
    auto video_track = m_pcf->CreateVideoTrack("video0", video_source_proxy.get());
    m_pc->AddTrack(video_track, {"stream0"});

    // ── AudioTrack ───────────────────────────────────────────────────────────
    auto audio_source = m_pcf->CreateAudioSource(cricket::AudioOptions{});
    auto audio_track  = m_pcf->CreateAudioTrack("audio0", audio_source.get());
    m_pc->AddTrack(audio_track, {"stream0"});

    // ── Créer l'offer SDP ────────────────────────────────────────────────────
    auto obs = rtc::make_ref_counted<CreateSdpObserver>(
        [this](webrtc::SessionDescriptionInterface *desc) {
            // Appliquer localement puis envoyer au signaling
            m_pc->SetLocalDescription(
                rtc::make_ref_counted<SetSdpObserver>().get(),
                desc);
            SdpMessage sdp;
            sdp.type = desc->type();
            desc->ToString(&sdp.sdp);
            LOGI("[WebRtcEngine] SDP offer généré (%zu octets)", sdp.sdp.size());
            if (m_signaling) m_signaling->sendSdp(sdp);
        },
        [](webrtc::RTCError e) {
            LOGE("[WebRtcEngine] CreateOffer failed: %s", e.message());
        });

    webrtc::PeerConnectionInterface::RTCOfferAnswerOptions opts;
    opts.offer_to_receive_audio = 0; // on ne reçoit pas audio du distant
    opts.offer_to_receive_video = 0; // on ne reçoit pas vidéo du distant
    m_pc->CreateOffer(obs.get(), opts);
#else
    LOGW("[WebRtcEngine] createOffer() stub — libwebrtc absent");
#endif
}

// ─── setRemoteSdp() ──────────────────────────────────────────────────────────

void WebRtcEngine::setRemoteSdp(const SdpMessage &sdp) {
    LOGI("[WebRtcEngine] setRemoteSdp type=%s", sdp.type.c_str());
#ifdef WEBRTC_ANDROID
    if (!m_pc) return;
    webrtc::SdpParseError error;
    std::unique_ptr<webrtc::SessionDescriptionInterface> desc =
        webrtc::CreateSessionDescription(sdp.type, sdp.sdp, &error);
    if (!desc) {
        LOGE("[WebRtcEngine] SDP parse error: %s", error.description.c_str());
        return;
    }
    m_pc->SetRemoteDescription(
        rtc::make_ref_counted<SetSdpObserver>().get(),
        desc.release());
#endif
}

// ─── addIceCandidate() ───────────────────────────────────────────────────────

void WebRtcEngine::addIceCandidate(const IceCandidate &candidate) {
    LOGD("[WebRtcEngine] addIceCandidate mid=%s", candidate.sdpMid.c_str());
#ifdef WEBRTC_ANDROID
    if (!m_pc) return;
    webrtc::SdpParseError error;
    std::unique_ptr<webrtc::IceCandidateInterface> ic(
        webrtc::CreateIceCandidate(
            candidate.sdpMid,
            candidate.sdpMLineIndex,
            candidate.candidate,
            &error));
    if (!ic) {
        LOGE("[WebRtcEngine] ICE parse error: %s", error.description.c_str());
        return;
    }
    m_pc->AddIceCandidate(ic.get());
#endif
}
