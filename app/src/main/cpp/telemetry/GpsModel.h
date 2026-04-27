// telemetry/GpsModel.h
// Modèle de données GPS simple, passé du thread UI Java via JNI.

#pragma once

#include <cstdint>

struct GpsModel {
    double   latitude   = 0.0;
    double   longitude  = 0.0;
    double   altitude   = 0.0;   // mètres
    float    accuracy   = 0.0f;  // mètres
    float    speed      = 0.0f;  // m/s
    float    bearing    = 0.0f;  // degrés, 0=Nord
    int64_t  timestamp  = 0;     // epoch ms
    bool     valid      = false;
};
