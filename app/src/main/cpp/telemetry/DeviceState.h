// telemetry/DeviceState.h
// État temps réel du device : GPS, batterie, réseau.
// Accessible en lecture depuis n'importe quel thread via les getters atomiques.

#pragma once

#include "GpsModel.h"
#include <atomic>
#include <mutex>

class DeviceState {
public:
    static DeviceState &instance();

    // GPS — mis à jour depuis le thread JNI
    void  updateGps(const GpsModel &gps);
    GpsModel gps() const;

    // Batterie [0-100]
    void  setBatteryLevel(int level);
    int   batteryLevel() const { return m_battery.load(); }

    // Signal réseau [-120, 0] dBm, 0 = inconnu
    void  setSignalDbm(int dbm);
    int   signalDbm() const { return m_signal_dbm.load(); }

private:
    DeviceState() = default;
    ~DeviceState() = default;
    DeviceState(const DeviceState &) = delete;
    DeviceState &operator=(const DeviceState &) = delete;

    mutable std::mutex m_gps_mutex;
    GpsModel           m_gps;
    std::atomic<int>   m_battery{-1};
    std::atomic<int>   m_signal_dbm{0};
};
