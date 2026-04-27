// CV_Manager.h
// Corrections : suppression des 'using namespace' dans le header,
// ajout mutex pour display_mat, suppression de l'IP hardcodée.

#ifndef EC_PROJET_CV_MANAGER_H
#define EC_PROJET_CV_MANAGER_H

#include "Util.h"
#include "Native_Camera.h"
#include "Image_Reader.h"
#include "Encoder.h"
#include "socket_client-h264.h"
#include "core/AppConfig.h"

#include <opencv2/opencv.hpp>
#include <android/native_window.h>

#include <thread>
#include <mutex>
#include <atomic>
#include <string>

class CV_Manager {
public:
    CV_Manager();
    ~CV_Manager();

    void SetNativeWindow(ANativeWindow *native_window);
    void SetUpCamera();
    void StartCameraLoop();
    void StopCameraLoop();
    void TearDownCamera();

    void FlipCamera();
    void HaltCamera();
    void RunCV();
    void ReleaseMats();

    void SetUpTCP();

    bool IsInitialized() const;
    void SetInitialized(bool value);

private:
    void CameraLoop();

    // Caméra
    ANativeWindow        *m_native_window  = nullptr;
    Native_Camera        *m_native_camera  = nullptr;
    Image_Reader         *m_image_reader   = nullptr;
    AImage               *m_image          = nullptr;
    ImageFormat           m_view;
    camera_type           m_selected_camera_type = BACK_CAMERA;

    // Thread
    std::thread           m_camera_thread;
    std::atomic<bool>     m_camera_thread_stopped{true};
    std::atomic<bool>     m_camera_ready{false};
    std::mutex            m_camera_mutex;

    // display_mat protégé par m_mat_mutex
    std::mutex            m_mat_mutex;
    cv::Mat               display_mat;

    // OpenCV temporaires
    cv::Mat frame_gray;
    cv::Mat grad_x, abs_grad_x;
    cv::Mat grad_y, abs_grad_y;
    cv::Mat detected_edges;
    cv::Mat thresh;
    cv::Mat kernel;
    cv::Point anchor;
    cv::Mat cleaned;
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;

    // CV flags
    bool scan_mode   = false;
    double total_t   = 0;
    clock_t start_t  = 0;

    // Transport
    Encoder           *m_encoder       = nullptr;
    SocketClientH264  *m_socket_h264   = nullptr;

    // Misc
    bool m_initialized = false;
};

#endif // EC_PROJET_CV_MANAGER_H
