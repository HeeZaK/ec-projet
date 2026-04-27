// webrtc/AudioController.h
// Gestion de la source audio NDK (OpenSL ES / AAudio) → AudioTrack WebRTC.

#pragma once

#include <cstdint>
#include <atomic>
#include <thread>

#ifdef WEBRTC_ANDROID
#include "api/media_stream_interface.h"
#include "api/audio_codecs/builtin_audio_decoder_factory.h"
#include "api/audio_codecs/builtin_audio_encoder_factory.h"
#endif

class AudioController {
public:
    AudioController()  = default;
    ~AudioController() { stop(); }

    // Démarre la capture audio NDK (OpenSL ES)
    bool start();
    void stop();

    bool isRunning() const { return m_running.load(); }

private:
    void captureLoop();

    std::atomic<bool> m_running{false};
    std::thread       m_capture_thread;

    // Paramètres audio WebRTC standard
    static constexpr int kSampleRate   = 16000; // 16 kHz
    static constexpr int kChannels     = 1;     // mono
    static constexpr int kFrameSamples = 160;   // 10ms @ 16kHz
};
