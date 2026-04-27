#include "JsonProtocol.h"
#include "core/Logger.h"
#include <sstream>

namespace JsonProtocol {

// ─── Helpers minimalistes ────────────────────────────────────────────────────

// Échappe les caractères spéciaux JSON dans une chaîne
static std::string jsonEscape(const std::string &s) {
    std::string out;
    out.reserve(s.size() + 16);
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:   out += c;      break;
        }
    }
    return out;
}

// Extrait la valeur string d'une clé dans un JSON simple (pas de nesting)
std::string extractString(const std::string &json, const std::string &key) {
    std::string needle = "\"" + key + "\"";
    size_t pos = json.find(needle);
    if (pos == std::string::npos) return "";
    // Avancer après la clé et les éventuels espaces/:  
    pos += needle.size();
    pos = json.find(':', pos);
    if (pos == std::string::npos) return "";
    pos = json.find('"', pos);
    if (pos == std::string::npos) return "";
    ++pos; // saute le " ouvrant
    std::string result;
    while (pos < json.size()) {
        char c = json[pos];
        if (c == '\\' && pos + 1 < json.size()) {
            char n = json[++pos];
            switch (n) {
                case '"':  result += '"';  break;
                case '\\': result += '\\'; break;
                case 'n':  result += '\n'; break;
                case 'r':  result += '\r'; break;
                case 't':  result += '\t'; break;
                default:   result += n;    break;
            }
        } else if (c == '"') {
            break;
        } else {
            result += c;
        }
        ++pos;
    }
    return result;
}

int extractInt(const std::string &json, const std::string &key, int defaultVal) {
    std::string needle = "\"" + key + "\"";
    size_t pos = json.find(needle);
    if (pos == std::string::npos) return defaultVal;
    pos += needle.size();
    pos = json.find(':', pos);
    if (pos == std::string::npos) return defaultVal;
    ++pos;
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) ++pos;
    if (pos >= json.size()) return defaultVal;
    try { return std::stoi(json.substr(pos)); }
    catch (...) { return defaultVal; }
}

std::string extractType(const std::string &json) {
    return extractString(json, "type");
}

// ─── Encodage ────────────────────────────────────────────────────────────────

std::string encodeSdp(const SdpMessage &sdp) {
    std::ostringstream oss;
    oss << "{\"type\":\"" << jsonEscape(sdp.type)
        << "\",\"sdp\":\""  << jsonEscape(sdp.sdp) << "\"}";
    return oss.str();
}

std::string encodeIce(const IceCandidate &ice) {
    std::ostringstream oss;
    oss << "{\"type\":\"candidate\""
        << ",\"candidate\":\""     << jsonEscape(ice.candidate) << "\""
        << ",\"sdpMid\":\""        << jsonEscape(ice.sdpMid)   << "\""
        << ",\"sdpMLineIndex\":"   << ice.sdpMLineIndex
        << "}";
    return oss.str();
}

// ─── Décodage ────────────────────────────────────────────────────────────────

SdpMessage decodeSdp(const std::string &json) {
    SdpMessage sdp;
    sdp.type = extractString(json, "type");
    sdp.sdp  = extractString(json, "sdp");
    return sdp;
}

IceCandidate decodeIce(const std::string &json) {
    IceCandidate ice;
    ice.candidate    = extractString(json, "candidate");
    ice.sdpMid       = extractString(json, "sdpMid");
    ice.sdpMLineIndex = extractInt(json,   "sdpMLineIndex", 0);
    return ice;
}

} // namespace JsonProtocol
