// webrtc/WebRtcEngine.h
// Moteur WebRTC NDK — façade sur libwebrtc.
// Structure complète ; implémentation à remplir quand libwebrtc est linké.
//
// Pour compiler libwebrtc pour Android :
//   https://webrtc.googlesource.com/src/+/refs/heads/main/docs/native-code/android/
// Ou utiliser un prebuild : github.com/crow-misia/libwebrtc-android

#pragma once

#include <string>
#include <functional>
#include <memory>

#include "signaling/SignalingClient.h"
#include "telemetry/TelemetryPublisher.h"

class WebRtcEngine {
public:
    WebRtcEngine();
    ~WebRtcEngine();

    // Initialise PeerConnectionFactory et les sources A/V
    bool init();
    void destroy();

    // Crée une PeerConnection et génère un SDP offer
    void createOffer();

    // Reçoit le SDP answer du serveur distant
    void setRemoteSdp(const SdpMessage &sdp);

    // Ajoute un ICE candidate distant
    void addIceCandidate(const IceCandidate &candidate);

    // Injecte une frame vidéo depuis CaptureEngine / MediaPipeline
    // (sera branché sur VideoTrackSource)
    // void pushVideoFrame(const cv::Mat &rgba, int64_t timestampUs);

    // Callbacks vers la couche applicative
    std::function<void(const SdpMessage &)>   onLocalSdp;
    std::function<void(const IceCandidate &)> onLocalIce;
    std::function<void(bool)>                 onConnectionStateChange;

private:
    // TODO: déclarer les membres libwebrtc ici quand la lib est disponible
    // rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> m_pcf;
    // rtc::scoped_refptr<webrtc::PeerConnectionInterface>        m_pc;
    // rtc::scoped_refptr<webrtc::VideoTrackInterface>             m_video_track;
    // rtc::scoped_refptr<webrtc::AudioTrackInterface>             m_audio_track;

    std::unique_ptr<SignalingClient>    m_signaling;
    std::unique_ptr<TelemetryPublisher> m_telemetry;

    bool m_initialized = false;
};
