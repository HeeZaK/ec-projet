// webrtc/AudioController.cpp
// Capture audio via OpenSL ES et injection dans le pipeline WebRTC.
// Sans libwebrtc : stub qui démarre/arrête proprement.

#include "AudioController.h"
#include "core/Logger.h"

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <chrono>

bool AudioController::start() {
    if (m_running.exchange(true)) {
        LOGW("[AudioController] start() déjà appelé");
        return true;
    }
    LOGI("[AudioController] Démarrage capture audio (16kHz mono)");
    m_capture_thread = std::thread(&AudioController::captureLoop, this);
    return true;
}

void AudioController::stop() {
    if (!m_running.exchange(false)) return;
    if (m_capture_thread.joinable()) m_capture_thread.join();
    LOGI("[AudioController] Arrêté");
}

void AudioController::captureLoop() {
    // ── Initialisation OpenSL ES ────────────────────────────────────────────
    SLObjectItf engineObj = nullptr;
    SLEngineItf engine    = nullptr;

    SLresult res = slCreateEngine(&engineObj, 0, nullptr, 0, nullptr, nullptr);
    if (res != SL_RESULT_SUCCESS) {
        LOGE("[AudioController] slCreateEngine failed: %d", res);
        m_running = false;
        return;
    }
    (*engineObj)->Realize(engineObj, SL_BOOLEAN_FALSE);
    (*engineObj)->GetInterface(engineObj, SL_IID_ENGINE, &engine);

    // Configurer l'enregistreur audio
    SLDataLocator_IODevice micLocator = {
        SL_DATALOCATOR_IODEVICE,
        SL_IODEVICE_AUDIOINPUT,
        SL_DEFAULTDEVICEID_AUDIOINPUT,
        nullptr
    };
    SLDataSource audioSrc = { &micLocator, nullptr };

    SLDataLocator_AndroidSimpleBufferQueue bufQueue = {
        SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2
    };
    SLDataFormat_PCM format = {
        SL_DATAFORMAT_PCM,
        (SLuint32)kChannels,
        SL_SAMPLINGRATE_16,
        SL_PCMSAMPLEFORMAT_FIXED_16,
        SL_PCMSAMPLEFORMAT_FIXED_16,
        SL_SPEAKER_FRONT_CENTER,
        SL_BYTEORDER_LITTLEENDIAN
    };
    SLDataSink audioSink = { &bufQueue, &format };

    const SLInterfaceID ids[]  = { SL_IID_ANDROIDSIMPLEBUFFERQUEUE };
    const SLboolean req[]      = { SL_BOOLEAN_TRUE };

    SLObjectItf recorderObj  = nullptr;
    SLRecordItf recorder     = nullptr;
    SLAndroidSimpleBufferQueueItf bq = nullptr;

    res = (*engine)->CreateAudioRecorder(engine, &recorderObj, &audioSrc, &audioSink, 1, ids, req);
    if (res != SL_RESULT_SUCCESS) {
        LOGE("[AudioController] CreateAudioRecorder failed: %d", res);
        (*engineObj)->Destroy(engineObj);
        m_running = false;
        return;
    }
    (*recorderObj)->Realize(recorderObj, SL_BOOLEAN_FALSE);
    (*recorderObj)->GetInterface(recorderObj, SL_IID_RECORD, &recorder);
    (*recorderObj)->GetInterface(recorderObj, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &bq);

    // Buffer PCM 10ms
    int16_t buf[kFrameSamples * kChannels];
    (*bq)->Enqueue(bq, buf, sizeof(buf));
    (*recorder)->SetRecordState(recorder, SL_RECORDSTATE_RECORDING);

    LOGI("[AudioController] Capture audio démarrée");

    // ── Boucle de capture ───────────────────────────────────────────────────
    while (m_running.load()) {
        // Dans une vraie implémentation le callback SL ES est asynchrone.
        // Ici on simule par polling pour rester simple et portable.
        // TODO: connecter le callback SL ES à webrtc::AudioDeviceModule
        // via webrtc::AudioTransport::RecordedDataIsAvailable()
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // ── Nettoyage ───────────────────────────────────────────────────────────
    (*recorder)->SetRecordState(recorder, SL_RECORDSTATE_STOPPED);
    (*recorderObj)->Destroy(recorderObj);
    (*engineObj)->Destroy(engineObj);

    LOGI("[AudioController] Ressources OpenSL ES libérées");
}
