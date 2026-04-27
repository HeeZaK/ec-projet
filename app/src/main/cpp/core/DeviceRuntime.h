// core/DeviceRuntime.h
// Singleton qui centralise l'état global du device :
// threads actifs, configuration runtime, lifecycle.

#pragma once

#include <atomic>
#include <mutex>
#include <string>

class DeviceRuntime {
public:
    static DeviceRuntime &instance();

    // Lifecycle
    void start();
    void stop();
    bool isRunning() const { return m_running.load(); }

    // État réseau
    void setConnected(bool v);    bool isConnected()  const;

    // Session ID (généré à chaque start)
    std::string sessionId() const;

private:
    DeviceRuntime() = default;
    ~DeviceRuntime() = default;
    DeviceRuntime(const DeviceRuntime &) = delete;
    DeviceRuntime &operator=(const DeviceRuntime &) = delete;

    std::atomic<bool>  m_running{false};
    std::atomic<bool>  m_connected{false};
    mutable std::mutex m_mutex;
    std::string        m_session_id;

    static std::string generateSessionId();
};
