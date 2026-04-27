// native-lib.cpp
// JNI Bridge — point d'entrée unique entre Android (Java) et le C++ NDK.
// Règle : pas de logique métier ici, uniquement dispatch.

#include <jni.h>
#include <string>

#include "CV_Manager.h"
#include "core/DeviceRuntime.h"
#include "telemetry/DeviceState.h"
#include "telemetry/GpsModel.h"
#include "webrtc/WebRtcEngine.h"
#include "security/AuthManager.h"

// Instances globales — lifecycle géré par onStart/onStop
static CV_Manager   app;
static WebRtcEngine webrtc;

// JavaVM globale pour les callbacks JNI asynchrones (ex: AuthManager Keystore)
static JavaVM *g_jvm = nullptr;

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void * /*reserved*/) {
    g_jvm = vm;
    LOGI("[JNI] JNI_OnLoad");
    return JNI_VERSION_1_6;
}

extern "C" {

// ─── Lifecycle ────────────────────────────────────────────────────────────────

JNIEXPORT void JNICALL
Java_com_example_ecproject_MainActivity_onStart(
        JNIEnv *env, jobject /*thiz*/) {
    LOGI("[JNI] onStart");
    DeviceRuntime::instance().start();
    // Passer l'env pour accéder à Android Keystore
    AuthManager::instance().init(env);
    webrtc.init();
}

JNIEXPORT void JNICALL
Java_com_example_ecproject_MainActivity_onStop(
        JNIEnv * /*env*/, jobject /*thiz*/) {
    LOGI("[JNI] onStop");
    app.TearDownCamera();
    webrtc.destroy();
    DeviceRuntime::instance().stop();
}

// ─── Surface ──────────────────────────────────────────────────────────────────

JNIEXPORT void JNICALL
Java_com_example_ecproject_MainActivity_setSurface(
        JNIEnv *env, jobject /*thiz*/,
        jobject surface, jint width, jint height) {
    LOGI("[JNI] setSurface %dx%d", width, height);
    ANativeWindow *window = ANativeWindow_fromSurface(env, surface);
    app.SetNativeWindow(window);
    app.SetUpCamera();
    app.SetUpTCP();
    app.StartCameraLoop();
}

JNIEXPORT void JNICALL
Java_com_example_ecproject_MainActivity_releaseSurface(
        JNIEnv * /*env*/, jobject /*thiz*/) {
    LOGI("[JNI] releaseSurface");
    app.TearDownCamera();
}

// ─── Caméra ───────────────────────────────────────────────────────────────────

JNIEXPORT void JNICALL
Java_com_example_ecproject_MainActivity_flipCamera(
        JNIEnv * /*env*/, jobject /*thiz*/) {
    app.FlipCamera();
}

// ─── GPS ──────────────────────────────────────────────────────────────────────

JNIEXPORT void JNICALL
Java_com_example_ecproject_MainActivity_onGpsUpdate(
        JNIEnv * /*env*/, jobject /*thiz*/,
        jdouble lat, jdouble lon, jdouble alt,
        jfloat  speed, jfloat bearing, jfloat accuracy,
        jlong   timestampMs) {
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

// ─── Batterie ─────────────────────────────────────────────────────────────────

JNIEXPORT void JNICALL
Java_com_example_ecproject_MainActivity_onBatteryUpdate(
        JNIEnv * /*env*/, jobject /*thiz*/, jint level) {
    DeviceState::instance().setBatteryLevel(level);
}

} // extern "C"
