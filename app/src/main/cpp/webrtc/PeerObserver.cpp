#include "PeerObserver.h"
#include "core/Logger.h"

#ifdef WEBRTC_ANDROID

void PeerObserver::OnSignalingChange(
        webrtc::PeerConnectionInterface::SignalingState state) {
    LOGI("[PeerObserver] SignalingState: %d", (int)state);
}

void PeerObserver::OnIceCandidate(
        const webrtc::IceCandidateInterface *candidate) {
    if (!candidate) return;
    IceCandidate ic;
    candidate->ToString(&ic.candidate);
    ic.sdpMid        = candidate->sdp_mid();
    ic.sdpMLineIndex = candidate->sdp_mline_index();
    LOGD("[PeerObserver] ICE local généré: %s", ic.sdpMid.c_str());
    if (onLocalIce) onLocalIce(ic);
}

void PeerObserver::OnIceConnectionChange(
        webrtc::PeerConnectionInterface::IceConnectionState state) {
    LOGI("[PeerObserver] IceConnectionState: %d", (int)state);
}

void PeerObserver::OnIceGatheringChange(
        webrtc::PeerConnectionInterface::IceGatheringState state) {
    LOGI("[PeerObserver] IceGatheringState: %d", (int)state);
}

void PeerObserver::OnConnectionChange(
        webrtc::PeerConnectionInterface::PeerConnectionState state) {
    using S = webrtc::PeerConnectionInterface::PeerConnectionState;
    bool connected = (state == S::kConnected);
    LOGI("[PeerObserver] ConnectionState: %d (connected=%d)", (int)state, connected);
    if (onConnectionChange) onConnectionChange(connected);
}

void PeerObserver::OnDataChannel(
        rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel) {
    LOGI("[PeerObserver] DataChannel reçu: %s", data_channel->label().c_str());
    // TODO: passer à WebRtcEngine via callback si nécessaire
}

#endif
