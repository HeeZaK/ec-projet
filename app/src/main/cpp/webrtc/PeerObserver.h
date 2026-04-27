// webrtc/PeerObserver.h
// Observer de la PeerConnection WebRTC.
// Reçoit les callbacks ICE, état connexion, tracks distants.

#pragma once

#include <functional>
#include "signaling/SignalingClient.h"

#ifdef WEBRTC_ANDROID
#include "api/peer_connection_interface.h"

class PeerObserver : public webrtc::PeerConnectionObserver {
public:
    // Callbacks vers WebRtcEngine
    std::function<void(const IceCandidate &)> onLocalIce;
    std::function<void(bool)>                 onConnectionChange;

    // webrtc::PeerConnectionObserver
    void OnSignalingChange(
        webrtc::PeerConnectionInterface::SignalingState state) override;
    void OnIceCandidate(
        const webrtc::IceCandidateInterface *candidate) override;
    void OnIceConnectionChange(
        webrtc::PeerConnectionInterface::IceConnectionState state) override;
    void OnIceGatheringChange(
        webrtc::PeerConnectionInterface::IceGatheringState state) override;
    void OnConnectionChange(
        webrtc::PeerConnectionInterface::PeerConnectionState state) override;
    void OnAddStream(
        rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override {}
    void OnRemoveStream(
        rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override {}
    void OnDataChannel(
        rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel) override;
    void OnRenegotiationNeeded() override {}
    void OnTrack(
        rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver) override {}
};

#else // Stub

class PeerObserver {
public:
    std::function<void(const IceCandidate &)> onLocalIce;
    std::function<void(bool)>                 onConnectionChange;
};

#endif
