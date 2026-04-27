#include "DataChannelController.h"
#include "core/Logger.h"

#ifdef WEBRTC_ANDROID

DataChannelController::DataChannelController(
        rtc::scoped_refptr<webrtc::DataChannelInterface> dc)
    : m_dc(std::move(dc)) {
    m_dc->RegisterObserver(this);
    LOGI("[DataChannel] Initialisé — label: %s", m_dc->label().c_str());
}

DataChannelController::~DataChannelController() {
    if (m_dc) m_dc->UnregisterObserver();
}

void DataChannelController::send(const std::string &payload) {
    if (!m_open || !m_dc) {
        LOGW("[DataChannel] send() ignoré — canal fermé");
        return;
    }
    webrtc::DataBuffer buf(
        rtc::CopyOnWriteBuffer(payload.data(), payload.size()),
        true /* binary=false, text */
    );
    if (!m_dc->Send(buf)) {
        LOGE("[DataChannel] send() échec");
    }
}

void DataChannelController::OnStateChange() {
    auto state = m_dc->state();
    m_open = (state == webrtc::DataChannelInterface::kOpen);
    LOGI("[DataChannel] State: %s",
         webrtc::DataChannelInterface::DataStateString(state));
}

void DataChannelController::OnMessage(const webrtc::DataBuffer &buffer) {
    std::string msg(buffer.data.data<char>(), buffer.data.size());
    LOGD("[DataChannel] Message reçu: %s", msg.c_str());
    if (onRemoteCommand) onRemoteCommand(msg);
}

#else // Stub

void DataChannelController::send(const std::string &payload) {
    LOGD("[DataChannel] send() stub: %s", payload.c_str());
}

#endif
