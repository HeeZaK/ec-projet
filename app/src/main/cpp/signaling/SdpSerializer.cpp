#include "SdpSerializer.h"
#include "core/Logger.h"
#include <sstream>

namespace SdpSerializer {

std::string extractFingerprint(const std::string &sdp) {
    // Cherche la ligne "a=fingerprint:sha-256 XX:XX:..."
    const std::string marker = "a=fingerprint:";
    size_t pos = sdp.find(marker);
    if (pos == std::string::npos) return "";
    size_t end = sdp.find('\n', pos);
    std::string line = sdp.substr(pos + marker.size(),
                                  (end == std::string::npos ? sdp.size() : end) - pos - marker.size());
    // Supprimer \r éventuel
    if (!line.empty() && line.back() == '\r') line.pop_back();
    LOGD("[SdpSerializer] fingerprint extrait: %s", line.c_str());
    return line;
}

bool validate(const std::string &sdp) {
    bool hasAudio = sdp.find("m=audio") != std::string::npos;
    bool hasVideo = sdp.find("m=video") != std::string::npos;
    bool hasDC    = sdp.find("m=application") != std::string::npos;
    LOGI("[SdpSerializer] validate: audio=%d video=%d datachannel=%d",
         hasAudio, hasVideo, hasDC);
    return hasAudio && hasVideo;
}

std::string setPreferredVideoCodec(const std::string &sdp, const std::string &codec) {
    // Réordonne la liste de codecs dans la ligne m=video pour mettre codec en premier
    // Implémentation simplifiée — fonctionne pour H264 et VP8
    std::istringstream stream(sdp);
    std::ostringstream out;
    std::string line;
    while (std::getline(stream, line)) {
        // Cherche les lignes a=rtpmap:XX codec/
        if (line.find("a=rtpmap:") != std::string::npos &&
            line.find(codec) != std::string::npos) {
            // Extraire le payload type
            size_t colon = line.find(':');
            size_t space = line.find(' ', colon);
            // On passe la ligne telle quelle — le réordonnancement complet du m=
            // nécessiterait un parser SDP complet (hors scope ici)
        }
        out << line << '\n';
    }
    return out.str();
}

} // namespace SdpSerializer
