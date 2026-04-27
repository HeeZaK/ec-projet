// webrtc/VideoTrackSource.h
// Adaptateur cv::Mat → webrtc::VideoFrame (I420).
// Reçoit les frames RGBA depuis CV_Manager et les injecte dans le VideoTrack WebRTC.
// Quand libwebrtc n'est pas disponible : stub compilable.

#pragma once

#include <cstdint>
#include <functional>
#include <mutex>

#ifdef WEBRTC_ANDROID
// Headers libwebrtc
#include "api/media_stream_interface.h"
#include "api/video/video_frame.h"
#include "api/video/i420_buffer.h"
#include "media/base/adapted_video_track_source.h"
#include "rtc_base/ref_counted_object.h"
#endif

// Structure frame interne — indépendante de libwebrtc
struct RawVideoFrame {
    const uint8_t *data_rgba = nullptr; // pointeur sur buffer RGBA
    int            width     = 0;
    int            height    = 0;
    int64_t        timestamp_us = 0;   // microsecondes
};

#ifdef WEBRTC_ANDROID

class VideoTrackSource
    : public rtc::RefCountedObject<cricket::AdaptedVideoTrackSource> {
public:
    VideoTrackSource() = default;
    ~VideoTrackSource() override = default;

    // Appelé par CV_Manager à chaque frame RGBA disponible
    void OnFrameAvailable(const RawVideoFrame &frame);

    // webrtc::MediaSourceInterface
    webrtc::MediaSourceInterface::SourceState state() const override {
        return webrtc::MediaSourceInterface::kLive;
    }
    bool remote() const override { return false; }
    bool is_screencast() const override { return false; }
    absl::optional<bool> needs_denoising() const override { return false; }

private:
    // Convertit RGBA → I420 dans le buffer fourni
    static void RgbaToI420(const uint8_t *rgba, int width, int height,
                            uint8_t *dst_y,  int stride_y,
                            uint8_t *dst_u,  int stride_u,
                            uint8_t *dst_v,  int stride_v);
};

#else // WEBRTC_ANDROID non défini : stub

class VideoTrackSource {
public:
    // Même signature — appel ignoré sans libwebrtc
    void OnFrameAvailable(const RawVideoFrame &frame);
};

#endif
