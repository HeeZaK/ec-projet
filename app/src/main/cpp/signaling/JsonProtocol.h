#pragma once
#include <string>
#include "SignalingClient.h"

// Encodage/décodage JSON minimaliste pour le protocole de signaling.
// Pas de dépendance externe (nlohmann/json) — parsing manuel léger.
// Format des messages :
//   SDP offer/answer : { "type": "offer"|"answer", "sdp": "<sdp brut>" }
//   ICE candidate   : { "type": "candidate", "candidate": "...", "sdpMid": "...", "sdpMLineIndex": N }
namespace JsonProtocol {
    std::string  encodeSdp(const SdpMessage &sdp);
    SdpMessage   decodeSdp(const std::string &json);

    std::string  encodeIce(const IceCandidate &ice);
    IceCandidate decodeIce(const std::string &json);

    // Extrait la valeur de la clé "type" dans un JSON plat
    std::string  extractType(const std::string &json);

    // Helpers
    std::string  extractString(const std::string &json, const std::string &key);
    int          extractInt(const std::string &json, const std::string &key, int defaultVal = 0);
}
