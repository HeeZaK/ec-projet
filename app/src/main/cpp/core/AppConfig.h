// core/AppConfig.h
// Configuration centralisée de l'application.
// L'IP et le port ne sont JAMAIS hardcodés dans les fichiers source.
//
// Pour changer la cible en dev : modifier ici uniquement,
// ou (mieux) injecter via BuildConfig depuis Gradle.

#pragma once

#include <cstdint>

namespace AppConfig {

    // ── Transport ─────────────────────────────────────────────────────────────
    // Adresse du serveur de réception vidéo / signaling.
    // À surcharger via CMake (-DSERVER_IP=...) ou variable d'env à la build.
#ifndef EC_SERVER_IP
    constexpr const char *SERVER_IP   = "127.0.0.1"; // loopback par défaut (safe)
#else
    constexpr const char *SERVER_IP   = EC_SERVER_IP;
#endif

#ifndef EC_SERVER_PORT
    constexpr uint16_t    SERVER_PORT = 8080;
#else
    constexpr uint16_t    SERVER_PORT = EC_SERVER_PORT;
#endif

    // ── Signaling ─────────────────────────────────────────────────────────────
#ifndef EC_SIGNALING_URL
    constexpr const char *SIGNALING_URL = "ws://127.0.0.1:8088/ws";
#else
    constexpr const char *SIGNALING_URL = EC_SIGNALING_URL;
#endif

    // ── Caméra ────────────────────────────────────────────────────────────────
    constexpr int         MAX_CAPTURE_WIDTH  = 1920;
    constexpr int         MAX_CAPTURE_HEIGHT = 1080;

    // ── Encoder ───────────────────────────────────────────────────────────────
    constexpr int         ENCODER_BITRATE_BPS = 2'000'000; // 2 Mbps
    constexpr int         ENCODER_FPS         = 30;

    // ── Telemetry ─────────────────────────────────────────────────────────────
    constexpr int         TELEMETRY_INTERVAL_MS = 1000; // publication GPS toutes les 1s

} // namespace AppConfig
