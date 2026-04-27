#pragma once
#include <string>
#include "SignalingClient.h"

// Utilitaires de manipulation de SDP brut (pas de JSON ici)
namespace SdpSerializer {
    // Extrait la ligne a=fingerprint du SDP pour le cert pinning
    std::string extractFingerprint(const std::string &sdp);

    // Vérifie que le SDP contient bien audio + vidéo + datachannel
    bool validate(const std::string &sdp);

    // Force le codec vidéo préféré (ex: "H264" ou "VP8")
    std::string setPreferredVideoCodec(const std::string &sdp, const std::string &codec);
}
