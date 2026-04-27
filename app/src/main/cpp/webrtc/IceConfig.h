// webrtc/IceConfig.h
// Configuration ICE/STUN/TURN pour la PeerConnection.
// Personnaliser les serveurs STUN/TURN selon l'infrastructure.

#pragma once

#include <vector>
#include <string>

#ifdef WEBRTC_ANDROID
#include "api/peer_connection_interface.h"

namespace IceConfig {

    // Serveurs STUN publics (fallback) + TURN à configurer
    inline std::vector<webrtc::PeerConnectionInterface::IceServer> getServers() {
        std::vector<webrtc::PeerConnectionInterface::IceServer> servers;

        // ── STUN Google (fallback public) ──────────────────────────────────
        webrtc::PeerConnectionInterface::IceServer stun1;
        stun1.uri = "stun:stun.l.google.com:19302";
        servers.push_back(stun1);

        webrtc::PeerConnectionInterface::IceServer stun2;
        stun2.uri = "stun:stun1.l.google.com:19302";
        servers.push_back(stun2);

        // ── TURN (à configurer avec votre serveur coturn) ──────────────────
        // webrtc::PeerConnectionInterface::IceServer turn;
        // turn.uri      = "turn:your-turn-server.example.com:3478";
        // turn.username = "username";
        // turn.password = "password";
        // servers.push_back(turn);

        return servers;
    }

    inline webrtc::PeerConnectionInterface::RTCConfiguration getConfig() {
        webrtc::PeerConnectionInterface::RTCConfiguration config;
        config.servers               = getServers();
        config.sdp_semantics         = webrtc::SdpSemantics::kUnifiedPlan;
        config.bundle_policy         = webrtc::PeerConnectionInterface::kBundlePolicyMaxBundle;
        config.rtcp_mux_policy       = webrtc::PeerConnectionInterface::kRtcpMuxPolicyRequire;
        config.ice_connection_receiving_timeout = 5000;
        return config;
    }

} // namespace IceConfig

#endif
