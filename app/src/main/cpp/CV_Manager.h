#ifndef ECPROJECT_CV_MANAGER_H
#define ECPROJECT_CV_MANAGER_H

#include <android/native_window.h>
#include <jni.h>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include "Image_Reader.h"
#include "Native_Camera.h"
#include "socket_client-h264.h"
#include "Encoder.h"
#include "Util.h"
#include <cstdlib>
#include <string>
#include <vector>
#include <thread>
#include <atomic>

using namespace cv;
using namespace std;

class CV_Manager {
public:
    CV_Manager();
    ~CV_Manager();

    // Configuration initiale
    void SetNativeWindow(ANativeWindow *native_window);
    void SetUpCamera();
    void SetUpTCP();

    // Boucles principales
    void CameraLoop();
    void NetworkLoop();

    // Actions utilisateur
    void RunCV();
    void HaltCamera();
    void FlipCamera();

private:
    // Utilitaires
    void ReleaseMats();
    void BarcodeDetect(Mat &frame);
    void BGR2YUV_nv12(Mat &src, Mat &dst);
    void convertYUV_I420toNV12(unsigned char* i420bytes, unsigned char* nv12bytes, int width, int height);

    // Camera & Affichage
    ANativeWindow *m_native_window;
    Native_Camera *m_native_camera;
    camera_type m_selected_camera_type = BACK_CAMERA;
    ImageFormat m_view{0, 0, 0};
    Image_Reader *m_image_reader;
    AImage *m_image;
    volatile bool m_camera_ready;
    bool m_camera_thread_stopped = false;

    // OpenCV
    std::atomic<bool> scan_mode{false};
    Mat display_mat, frame_gray, detected_edges;
    vector<vector<Point>> contours;
    vector<Vec4i> hierarchy;

    // Réseau & Encodage (Code du prof)
    SocketClientH264* m_socket_h264;
    Encoder* m_encoder;
    bool m_is_connected = false;
};

#endif //ECPROJECT_CV_MANAGER_H
