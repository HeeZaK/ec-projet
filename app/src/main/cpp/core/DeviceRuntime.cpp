// core/DeviceRuntime.cpp

#include "DeviceRuntime.h"
#include "Util.h"

#include <chrono>
#include <sstream>
#include <iomanip>
#include <random>

DeviceRuntime &DeviceRuntime::instance() {
    static DeviceRuntime inst;
    return inst;
}

void DeviceRuntime::start() {
    std::lock_guard<std::mutex> lk(m_mutex);
    if (m_running.exchange(true)) {
        LOGW("[DeviceRuntime] start() appelé alors que déjà en cours");
        return;
    }
    m_session_id = generateSessionId();
    LOGI("[DeviceRuntime] Démarré — session: %s", m_session_id.c_str());
}

void DeviceRuntime::stop() {
    if (!m_running.exchange(false)) {
        LOGW("[DeviceRuntime] stop() appelé alors qu'inactif");
        return;
    }
    m_connected = false;
    LOGI("[DeviceRuntime] Arrêté");
}

void DeviceRuntime::setConnected(bool v) {
    m_connected.store(v);
    LOGI("[DeviceRuntime] Connexion: %s", v ? "établie" : "perdue");
}

bool DeviceRuntime::isConnected() const { return m_connected.load(); }

std::string DeviceRuntime::sessionId() const {
    std::lock_guard<std::mutex> lk(m_mutex);
    return m_session_id;
}

std::string DeviceRuntime::generateSessionId() {
    using namespace std::chrono;
    auto ts = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    std::mt19937_64 rng(static_cast<uint64_t>(ts));
    std::uniform_int_distribution<uint64_t> dist;
    std::ostringstream oss;
    oss << std::hex << std::setfill('0')
        << std::setw(16) << dist(rng)
        << std::setw(16) << dist(rng);
    return oss.str();
}
