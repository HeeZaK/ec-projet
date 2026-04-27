#include <jni.h>
#include <string>
#include <android/asset_manager_jni.h>
#include <android/native_window_jni.h>
#include <thread>
#include "CV_Manager.h"

static CV_Manager app;

extern "C"
JNIEXPORT void JNICALL
Java_com_example_ecproject_MainActivity_scan(JNIEnv *env, jobject thiz) {
    // TODO: implement scan()
    app.RunCV();
}
extern "C"
JNIEXPORT void JNICALL
Java_com_example_ecproject_MainActivity_flipCamera(JNIEnv *env, jobject thiz) {
    // TODO: implement flipCamera()
    // TODO: implement flipCamera()
    //app.HaltCamera();
    app.FlipCamera();
}
extern "C"
JNIEXPORT void JNICALL
Java_com_example_ecproject_MainActivity_setSurface(JNIEnv *env, jobject thiz, jobject surface) {
    // TODO: implement setSurface()
    // Obtention du native window depuis la surface Java
    ANativeWindow* window = ANativeWindow_fromSurface(env, surface);
    app.SetNativeWindow(window);
    //app.SetUpTCP();
    // Configuration de la caméra
    app.SetUpCamera();

    // Insertion d'un court délai pour s'assurer de la bonne initialisation des ressources
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Démarrage de la boucle de capture dans un thread détaché
    std::thread loopThread(&CV_Manager::CameraLoop, &app);
    loopThread.detach();

}