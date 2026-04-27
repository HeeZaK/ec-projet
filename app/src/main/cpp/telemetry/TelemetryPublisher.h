// telemetry/TelemetryPublisher.h
// Publie périodiquement les données de telemetry (GPS, batterie, signal)
// vers le DataChannel WebRTC (ou fallback socket) en JSON compact.

#pragma once

#include <thread>
#include <atomic>
#include <functional>
#include <string>

class TelemetryPublisher {
public:
    // sendFn : callback appelé avec le JSON sérialisé à chaque publication.
    // Connecter à DataChannelController::send() une fois WebRTC implémenté.
    explicit TelemetryPublisher(std::function<void(const std::string &)> sendFn);
    ~TelemetryPublisher();

    void start(int intervalMs = 1000);
    void stop();

private:
    void publishLoop(int intervalMs);
    std::string buildPayload() const;

    std::function<void(const std::string &)> m_send_fn;
    std::thread      m_thread;
    std::atomic<bool> m_running{false};
};
