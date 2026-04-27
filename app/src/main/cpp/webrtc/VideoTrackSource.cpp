// webrtc/VideoTrackSource.cpp
// Conversion RGBA → I420 + injection dans le pipeline WebRTC.

#include "VideoTrackSource.h"
#include "core/Logger.h"

#ifdef WEBRTC_ANDROID

void VideoTrackSource::OnFrameAvailable(const RawVideoFrame &frame) {
    if (!frame.data_rgba || frame.width <= 0 || frame.height <= 0) return;

    // Allouer un buffer I420
    rtc::scoped_refptr<webrtc::I420Buffer> i420 =
        webrtc::I420Buffer::Create(frame.width, frame.height);

    RgbaToI420(
        frame.data_rgba, frame.width, frame.height,
        i420->MutableDataY(), i420->StrideY(),
        i420->MutableDataU(), i420->StrideU(),
        i420->MutableDataV(), i420->StrideV()
    );

    webrtc::VideoFrame wrtc_frame =
        webrtc::VideoFrame::Builder()
            .set_video_frame_buffer(i420)
            .set_timestamp_us(frame.timestamp_us)
            .set_rotation(webrtc::kVideoRotation_0)
            .build();

    // Injecter dans le pipeline WebRTC
    OnFrame(wrtc_frame);
}

void VideoTrackSource::RgbaToI420(
        const uint8_t *rgba, int width, int height,
        uint8_t *dst_y,  int stride_y,
        uint8_t *dst_u,  int stride_u,
        uint8_t *dst_v,  int stride_v) {

    // Conversion RGBA → YUV I420 (BT.601)
    // Y  =  0.299*R + 0.587*G + 0.114*B
    // Cb = -0.169*R - 0.331*G + 0.500*B + 128
    // Cr =  0.500*R - 0.419*G - 0.081*B + 128

    for (int y = 0; y < height; ++y) {
        const uint8_t *row = rgba + y * width * 4;
        uint8_t *dst_y_row  = dst_y + y * stride_y;

        for (int x = 0; x < width; ++x) {
            uint8_t r = row[x * 4 + 0];
            uint8_t g = row[x * 4 + 1];
            uint8_t b = row[x * 4 + 2];

            // Luma (Y)
            dst_y_row[x] = (uint8_t)((77 * r + 150 * g + 29 * b) >> 8);

            // Chrominance (sous-échantillonnage 4:2:0 — un pixel UV pour 2×2 luma)
            if ((y & 1) == 0 && (x & 1) == 0) {
                int uv_row = y / 2;
                int uv_col = x / 2;
                dst_u[uv_row * stride_u + uv_col] =
                    (uint8_t)((((-43 * r - 85 * g + 128 * b) >> 8) + 128));
                dst_v[uv_row * stride_v + uv_col] =
                    (uint8_t)((((128 * r - 107 * g - 21 * b) >> 8) + 128));
            }
        }
    }
}

#else // Stub sans libwebrtc

void VideoTrackSource::OnFrameAvailable(const RawVideoFrame &frame) {
    (void)frame;
    // Stub — rien à faire sans libwebrtc
}

#endif
