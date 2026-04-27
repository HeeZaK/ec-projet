// telemetry/DeviceState.cpp

#include "DeviceState.h"

DeviceState &DeviceState::instance() {
    static DeviceState inst;
    return inst;
}

void DeviceState::updateGps(const GpsModel &gps) {
    std::lock_guard<std::mutex> lk(m_gps_mutex);
    m_gps = gps;
}

GpsModel DeviceState::gps() const {
    std::lock_guard<std::mutex> lk(m_gps_mutex);
    return m_gps;
}

void DeviceState::setBatteryLevel(int level) { m_battery.store(level); }
void DeviceState::setSignalDbm(int dbm)      { m_signal_dbm.store(dbm); }
