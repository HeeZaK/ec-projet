// core/Logger.h
// Macros de log NDK uniformes.
// Utiliser ces macros partout à la place de __android_log_print direct.

#pragma once

#include <android/log.h>

#ifndef LOG_TAG
#define LOG_TAG "ECProject"
#endif

#define LOGV(fmt, ...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGD(fmt, ...) __android_log_print(ANDROID_LOG_DEBUG,   LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGI(fmt, ...) __android_log_print(ANDROID_LOG_INFO,    LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...) __android_log_print(ANDROID_LOG_WARN,    LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGE(fmt, ...) __android_log_print(ANDROID_LOG_ERROR,   LOG_TAG, fmt, ##__VA_ARGS__)

// Assertion NDK : log + abort si condition fausse
#define ASSERT(cond, fmt, ...)                                      \
    do {                                                            \
        if (!(cond)) {                                              \
            __android_log_print(ANDROID_LOG_FATAL, LOG_TAG,        \
                "ASSERTION FAILED @ %s:%d: " fmt,                  \
                __FILE__, __LINE__, ##__VA_ARGS__);                 \
            __builtin_trap();                                       \
        }                                                           \
    } while (0)
