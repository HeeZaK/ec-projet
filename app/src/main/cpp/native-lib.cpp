// native-lib.cpp
// JNI Bridge — point d'entrée unique entre Android (Java/Kotlin) et le C++ NDK.
// Règle : pas de logique métier ici, uniquement dispatch vers les managers.

#include <jni.h>
#include <string>
#include "CV_Manager.h"
#include "core/DeviceRuntime.h"
#include "telemetry/DeviceState.h"
#include "webrtc/WebRtcEngine.h"
#include "security/AuthManager.h"

// Instance globale unique — protégée par le mutex de DeviceRuntime
static CV_Manager    app;
static WebRtcEngine  webrtc;

extern "C" {

// ─── Lifecycle ────────────────────────────────────────────────────────────────

JNIEXPORT void JNICALL
Java_com_example_ecproject_MainActivity_onStart(JNIEnv *env, jobject /*this*/) {
    DeviceRuntime::instance().start();
    AuthManager::instance().init();
    webrtc.init();
}

JNIEXPORT void JNICALL
Java_com_example_ecproject_MainActivity_onStop(JNIEnv *env, jobject /*this*/) {
    app.TearDownCamera();
    webrtc.destroy();
    DeviceRuntime::instance().stop();
}

// ─── Surface ──────────────────────────────────────────────────────────────────

JNIEXPORT void JNICALL
Java_com_example_ecproject_MainActivity_setSurface(
        JNIEnv *env, jobject /*this*/, jobject surface, jint width, jint height) {
    ANativeWindow *window = ANativeWindow_fromSurface(env, surface);
    app.SetNativeWindow(window);
    app.SetUpCamera();
    app.SetUpTCP();
    app.StartCameraLoop();
}

JNIEXPORT void JNICALL
Java_com_example_ecproject_MainActivity_releaseSurface(
        JNIEnv *env, jobject /*this*/) {
    app.TearDownCamera();
}

// ─── Caméra ───────────────────────────────────────────────────────────────────

JNIEXPORT void JNICALL
Java_com_example_ecproject_MainActivity_flipCamera(
        JNIEnv *env, jobject /*this*/) {
    app.FlipCamera();
}

// ─── Télémétrie GPS (appelé depuis LocationManager Java) ─────────────────────

JNIEXPORT void JNICALL
Java_com_example_ecproject_MainActivity_onGpsUpdate(
        JNIEnv *env, jobject /*this*/,
        jdouble lat, jdouble lon, jdouble alt,
        jfloat speed, jfloat bearing, jfloat accuracy,
        jlong timestampMs) {
    GpsModel gps;
    gps.latitude   = lat;
    gps.longitude  = lon;
    gps.altitude   = alt;
    gps.speed      = speed;
    gps.bearing    = bearing;
    gps.accuracy   = accuracy;
    gps.timestamp  = timestampMs;
    gps.valid      = true;
    DeviceState::instance().updateGps(gps);
}

// ─── Infos device ─────────────────────────────────────────────────────────────

JNIEXPORT void JNICALL
Java_com_example_ecproject_MainActivity_onBatteryUpdate(
        JNIEnv *env, jobject /*this*/, jint level) {
    DeviceState::instance().setBatteryLevel(level);
}

} // extern "C"
