//
// Created by ladiaviakoye on 02/03/2022.
//

#include "Encoder.h"
#include "socket_client-h264.h"
#include <android/log.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <thread>
#include <mutex>
#include <iostream>

// Augmentation du timeout pour laisser le temps au codec de libérer des buffers (10ms)
#define TIMEOUT_US  10000

#define NSEC_PER_SEC 1000000000

static uint64_t startTime = 0;

static inline uint64_t timespecToNsec(const struct timespec *a)
{
    return (uint64_t)a->tv_sec * NSEC_PER_SEC + a->tv_nsec;
}

static uint64_t getTime()
{
    struct timespec tp;
    clock_gettime(CLOCK_MONOTONIC, &tp);
    return  timespecToNsec(&tp);
}

static void* s_handleOutput(void* data) {
    Encoder* encoder = (Encoder*)data;
    encoder->handleOutput();
    return NULL;
}

Encoder::Encoder() :
        mHeaderBuf(NULL), mSizeofHeader(0), mMediaCodec(NULL), mMediaFormat(NULL), mClienth264(NULL)
{
};

void Encoder::handleOutput()
{
    AMediaCodecBufferInfo info;
    size_t                size;
    uint8_t*              outBuf;
    ssize_t               outBufId;

    __android_log_write(ANDROID_LOG_DEBUG, "EncoderNDK", "handleOutput thread started");

    while (mMediaCodec != NULL) {
        outBufId = AMediaCodec_dequeueOutputBuffer(mMediaCodec, &info, TIMEOUT_US);

        if (outBufId == AMEDIACODEC_INFO_TRY_AGAIN_LATER) {
            continue;
        } else if (outBufId == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED) {
            __android_log_write(ANDROID_LOG_DEBUG, "EncoderNDK", "Output format changed");
            continue;
        } else if (outBufId < 0) {
            continue;
        }

        outBuf = AMediaCodec_getOutputBuffer(mMediaCodec, outBufId, &size);
        if (outBuf && info.size > 0 && mClienth264 && mClienth264->m_is_connected) {
            this->mClienth264->SendImageH264(outBuf, info.size);
        }

        AMediaCodec_releaseOutputBuffer(mMediaCodec, outBufId, false);
    }
    __android_log_write(ANDROID_LOG_DEBUG, "EncoderNDK", "handleOutput thread exiting");
}

void Encoder::InitCodec(int height , int width, int framerate ,int bitrate)
{
    mheight = height;
    mwidth = width;
    mMediaCodec = AMediaCodec_createEncoderByType("video/avc");

    if (mMediaCodec == NULL) {
        __android_log_write(ANDROID_LOG_ERROR, "EncoderNDK", "Unable to create an encoder for video/avc");
        return;
    }

    mMediaFormat   = AMediaFormat_new();
    AMediaFormat_setString(mMediaFormat, AMEDIAFORMAT_KEY_MIME, "video/avc");
    AMediaFormat_setInt32(mMediaFormat, AMEDIAFORMAT_KEY_WIDTH, mwidth);
    AMediaFormat_setInt32(mMediaFormat, AMEDIAFORMAT_KEY_HEIGHT, mheight);
    AMediaFormat_setInt32(mMediaFormat, AMEDIAFORMAT_KEY_FRAME_RATE, framerate);
    AMediaFormat_setInt32(mMediaFormat, AMEDIAFORMAT_KEY_BIT_RATE, bitrate);
    AMediaFormat_setInt32(mMediaFormat, AMEDIAFORMAT_KEY_I_FRAME_INTERVAL, 5);
    AMediaFormat_setInt32(mMediaFormat, AMEDIAFORMAT_KEY_COLOR_FORMAT, 21); // NV12

    mStatus = AMediaCodec_configure(mMediaCodec, mMediaFormat, nullptr, nullptr, AMEDIACODEC_CONFIGURE_FLAG_ENCODE);

    if (mStatus != AMEDIA_OK) {
        __android_log_print(ANDROID_LOG_ERROR,"EncoderNDK","AMediaCodec_configure() failed: %d", (int)mStatus);
    } else {
        if ((mStatus = AMediaCodec_start(mMediaCodec)) != AMEDIA_OK) {
            __android_log_print(ANDROID_LOG_ERROR, " EncoderNDK ", "AMediaCodec_start failed");
        }
    }

    AMediaFormat_delete(mMediaFormat);
    pthread_create(&mDecoderThread, NULL, s_handleOutput, this);
}

Encoder::~Encoder()
{
    AMediaCodec* codec = mMediaCodec;
    mMediaCodec = NULL; // Signal pour arrêter la boucle handleOutput

    if (codec) {
        AMediaCodec_stop(codec);
        AMediaCodec_delete(codec);
    }
}

void Encoder::Encode (unsigned char *YUV_NV12, int planeSize)
{
    if (!mMediaCodec) return;

    size_t   size;
    uint8_t* bufferPointer;
    uint64_t currentTime;
    ssize_t inputBufId;

    if (startTime == 0) startTime = getTime();
    currentTime = (getTime() - startTime) / 1000; // microsecondes

    inputBufId = AMediaCodec_dequeueInputBuffer(mMediaCodec, TIMEOUT_US);

    if (inputBufId == AMEDIACODEC_INFO_TRY_AGAIN_LATER) {
        return; // Ignore silencieusement si l'encodeur est occupé
    }

    if (inputBufId >= 0) {
        bufferPointer = AMediaCodec_getInputBuffer(mMediaCodec, static_cast<size_t>(inputBufId), &size);
        if (bufferPointer) {
            size_t bytesToCopy = (size < (size_t)planeSize) ? size : (size_t)planeSize;
            memcpy(bufferPointer, YUV_NV12, bytesToCopy);
            mStatus = AMediaCodec_queueInputBuffer(mMediaCodec, static_cast<size_t>(inputBufId), 0, bytesToCopy, currentTime, 0);
        }
    } else {
        __android_log_print(ANDROID_LOG_ERROR, "EncoderNDK", "Dequeue input buffer failed: %zd", inputBufId);
    }
}

media_status_t Encoder::getStatus() { return mStatus; }

void Encoder::setSocketClientH264(SocketClientH264 *m_ClientH264) { this->mClienth264 = m_ClientH264; }

bool Encoder::initFile(const char* H, const char* J, struct tm* t, time_t ts) { return false; }

bool Encoder::writeFile(uint8_t* B, int32_t s, size_t c) { return false; }
