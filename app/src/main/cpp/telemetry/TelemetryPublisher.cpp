// telemetry/TelemetryPublisher.cpp

#include "TelemetryPublisher.h"
#include "DeviceState.h"
#include "Util.h"

#include <chrono>
#include <sstream>
#include <iomanip>
#include <thread>

TelemetryPublisher::TelemetryPublisher(
    std::function<void(const std::string &)> sendFn)
    : m_send_fn(std::move(sendFn)) {}

TelemetryPublisher::~TelemetryPublisher() { stop(); }

void TelemetryPublisher::start(int intervalMs) {
    if (m_running.exchange(true)) return;
    m_thread = std::thread(&TelemetryPublisher::publishLoop, this, intervalMs);
    LOGI("[TelemetryPublisher] Démarré (interval=%dms)", intervalMs);
}

void TelemetryPublisher::stop() {
    if (!m_running.exchange(false)) return;
    if (m_thread.joinable()) m_thread.join();
    LOGI("[TelemetryPublisher] Arrêté");
}

void TelemetryPublisher::publishLoop(int intervalMs) {
    while (m_running.load()) {
        std::string payload = buildPayload();
        if (m_send_fn) m_send_fn(payload);
        std::this_thread::sleep_for(std::chrono::milliseconds(intervalMs));
    }
}

std::string TelemetryPublisher::buildPayload() const {
    const DeviceState &ds = DeviceState::instance();
    GpsModel gps          = ds.gps();
    int battery           = ds.batteryLevel();
    int signal            = ds.signalDbm();

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(7);
    oss << "{";
    oss << "\"type\":\"telemetry\",";
    oss << "\"ts\":"    << gps.timestamp << ",";
    if (gps.valid) {
        oss << "\"lat\":"  << gps.latitude  << ",";
        oss << "\"lon\":"  << gps.longitude << ",";
        oss << "\"alt\":"  << gps.altitude  << ",";
        oss << "\"spd\":"  << gps.speed     << ",";
        oss << "\"hdg\":"  << gps.bearing   << ",";
        oss << "\"acc\":"  << gps.accuracy  << ",";
    }
    oss << "\"bat\":"  << battery << ",";
    oss << "\"sig\":"  << signal;
    oss << "}";
    return oss.str();
}
