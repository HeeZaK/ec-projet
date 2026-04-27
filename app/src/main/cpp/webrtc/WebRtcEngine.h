// webrtc/WebRtcEngine.h
// Moteur WebRTC NDK — façade sur libwebrtc.
// Gère : PeerConnection, VideoTrackSource, AudioController, DataChannel, Signaling.

#pragma once

#include <string>
#include <functional>
#include <memory>

#include "signaling/SignalingClient.h"
#include "telemetry/TelemetryPublisher.h"
#include "VideoTrackSource.h"
#include "AudioController.h"
#include "DataChannelController.h"
#include "PeerObserver.h"

#ifdef WEBRTC_ANDROID
#include "api/peer_connection_interface.h"
#include "api/audio_codecs/builtin_audio_decoder_factory.h"
#include "api/audio_codecs/builtin_audio_encoder_factory.h"
#include "api/video_codecs/builtin_video_decoder_factory.h"
#include "api/video_codecs/builtin_video_encoder_factory.h"
#include "rtc_base/thread.h"
#include "rtc_base/ssl_adapter.h"
#endif

class WebRtcEngine {
public:
    WebRtcEngine();
    ~WebRtcEngine();

    bool init();
    void destroy();

    // Reçoit une frame RGBA depuis CV_Manager → injecte dans VideoTrackSource
    void pushVideoFrame(const uint8_t *rgba, int width, int height, int64_t timestampUs);

    // Reçu du Signaling
    void setRemoteSdp(const SdpMessage &sdp);
    void addIceCandidate(const IceCandidate &candidate);

    // Callbacks
    std::function<void(bool)> onConnectionStateChange;

private:
    void createOffer();

    std::unique_ptr<SignalingClient>         m_signaling;
    std::unique_ptr<TelemetryPublisher>      m_telemetry;
    std::unique_ptr<AudioController>         m_audio;
    std::unique_ptr<PeerObserver>            m_peer_observer;

#ifdef WEBRTC_ANDROID
    rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> m_pcf;
    rtc::scoped_refptr<webrtc::PeerConnectionInterface>        m_pc;
    rtc::scoped_refptr<VideoTrackSource>                       m_video_source;
    std::unique_ptr<rtc::Thread>                               m_network_thread;
    std::unique_ptr<rtc::Thread>                               m_worker_thread;
    std::unique_ptr<rtc::Thread>                               m_signaling_thread;
    std::unique_ptr<DataChannelController>                     m_data_channel;
#else
    // Stub : VideoTrackSource sans libwebrtc
    std::unique_ptr<VideoTrackSource>                          m_video_source;
#endif

    bool m_initialized = false;
};
