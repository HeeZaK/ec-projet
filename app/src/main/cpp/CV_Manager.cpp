#include "CV_Manager.h"
#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>

using namespace std;
using namespace cv;

// Synchronisation pour le thread réseau
static std::mutex g_frame_mutex;
static std::condition_variable g_frame_cv;
static cv::Mat g_frame_to_send;
static bool g_network_thread_running = false;

CV_Manager::CV_Manager()
    : m_native_window(nullptr), m_native_camera(nullptr), m_image_reader(nullptr),
      m_image(nullptr), m_camera_ready(false), m_socket_h264(nullptr), m_encoder(nullptr), m_is_connected(false) {
}

CV_Manager::~CV_Manager() {
    g_network_thread_running = false;
    g_frame_cv.notify_all();

    if (m_encoder) delete m_encoder;
    if (m_socket_h264) delete m_socket_h264;
    if (m_native_camera) delete m_native_camera;
    if (m_native_window) ANativeWindow_release(m_native_window);
    if (m_image_reader) delete m_image_reader;
}

void CV_Manager::SetNativeWindow(ANativeWindow *native_window) {
    m_native_window = native_window;
}

void CV_Manager::SetUpCamera() {
    LOGI("CV_Manager: Configuration Caméra...");
    m_native_camera = new Native_Camera(m_selected_camera_type);

    // Déterminer la résolution du capteur
    m_native_camera->MatchCaptureSizeRequest(&m_view,
                                             ANativeWindow_getWidth(m_native_window),
                                             ANativeWindow_getHeight(m_native_window));

    // Ajuster la surface pour le mode Portrait
    int32_t w = m_view.width;
    int32_t h = m_view.height;
    if (m_native_camera->GetOrientation() % 180 != 0) {
        w = m_view.height;
        h = m_view.width;
    }
    ANativeWindow_setBuffersGeometry(m_native_window, w, h, WINDOW_FORMAT_RGBX_8888);

    // Initialiser le lecteur d'images (YUV)
    m_image_reader = new Image_Reader(&m_view, AIMAGE_FORMAT_YUV_420_888);
    m_image_reader->SetPresentRotation(m_native_camera->GetOrientation());
    m_camera_ready = m_native_camera->CreateCaptureSession(m_image_reader->GetNativeWindow());

    // Configurer l'encodeur avec les dimensions RÉELLES du buffer (après rotation possible)
    if (m_encoder) delete m_encoder;
    m_encoder = new Encoder();
    // InitCodec(height, width, fps, bitrate)
    m_encoder->InitCodec(h, w, 30, 2000000);

    SetUpTCP();
}

void CV_Manager::SetUpTCP() {
    if (m_socket_h264) delete m_socket_h264;
    m_socket_h264 = new SocketClientH264("82.64.44.67", 8080);
    m_encoder->setSocketClientH264(m_socket_h264);

    if (!g_network_thread_running) {
        g_network_thread_running = true;
        std::thread(&CV_Manager::NetworkLoop, this).detach();
    }

    std::thread([this]() {
        m_is_connected = m_socket_h264->ConnectToServer();
    }).detach();
}

void CV_Manager::NetworkLoop() {
    while (g_network_thread_running) {
        cv::Mat frame;
        {
            std::unique_lock<std::mutex> lock(g_frame_mutex);
            g_frame_cv.wait(lock, [] { return !g_frame_to_send.empty() || !g_network_thread_running; });
            if (!g_network_thread_running) break;
            g_frame_to_send.copyTo(frame);
            g_frame_to_send.release();
        }

        if (m_is_connected && m_encoder && !frame.empty()) {
            // Conversion BGR -> NV12 pour l'encodeur
            cv::Mat nv12;
            BGR2YUV_nv12(frame, nv12);
            // On envoie la taille totale en octets : width * height * 1.5
            m_encoder->Encode(nv12.data, nv12.total() * nv12.elemSize());
        }
    }
}

void CV_Manager::CameraLoop() {
    while (!m_camera_thread_stopped) {
        if (!m_camera_ready || !m_image_reader) continue;

        m_image = m_image_reader->GetLatestImage();
        if (m_image == nullptr) continue;

        ANativeWindow_acquire(m_native_window);
        ANativeWindow_Buffer buffer;
        if (ANativeWindow_lock(m_native_window, &buffer, nullptr) < 0) {
            m_image_reader->DeleteImage(m_image);
            ANativeWindow_release(m_native_window);
            continue;
        }

        m_image_reader->DisplayImage(&buffer, m_image);
        m_image = nullptr;

        // On utilise buffer.width et buffer.stride pour créer une Mat correcte (sans padding dans l'image utile)
        display_mat = Mat(buffer.height, buffer.width, CV_8UC4, buffer.bits, buffer.stride * 4);

        // Envoyer à l'encodeur via la file d'attente
        if (m_is_connected) {
            std::unique_lock<std::mutex> lock(g_frame_mutex, std::try_to_lock);
            if (lock.owns_lock()) {
                display_mat.copyTo(g_frame_to_send);
                g_frame_cv.notify_one();
            }
        }

        if (scan_mode) BarcodeDetect(display_mat);

        ANativeWindow_unlockAndPost(m_native_window);
        ANativeWindow_release(m_native_window);
        ReleaseMats();
    }
}

// --- Fonctions Utilitaires ---

void CV_Manager::BGR2YUV_nv12(Mat &src, Mat &dst) {
    Mat yuv_i420;
    // Note: src est RGBA (venant de WINDOW_FORMAT_RGBX_8888)
    cvtColor(src, yuv_i420, COLOR_RGBA2YUV_I420);
    dst = Mat(src.rows * 1.5, src.cols, CV_8UC1);
    convertYUV_I420toNV12(yuv_i420.data, dst.data, src.cols, src.rows);
}

void CV_Manager::convertYUV_I420toNV12(unsigned char* i420bytes, unsigned char* nv12bytes, int width, int height) {
    int nLenY = width * height;
    int nLenU = nLenY / 4;
    memcpy(nv12bytes, i420bytes, nLenY);
    for (int i = 0; i < nLenU; i++) {
        nv12bytes[nLenY + 2 * i] = i420bytes[nLenY + i];
        nv12bytes[nLenY + 2 * i + 1] = i420bytes[nLenY + nLenU + i];
    }
}

void CV_Manager::BarcodeDetect(Mat &frame) {
    if (frame.empty()) return;
    cvtColor(frame, frame_gray, COLOR_RGBA2GRAY);
    Canny(frame_gray, detected_edges, 50, 150);
    findContours(detected_edges, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
    drawContours(frame, contours, -1, Scalar(0, 255, 0), 2);
}

void CV_Manager::RunCV() { scan_mode = !scan_mode; }
void CV_Manager::HaltCamera() { m_camera_thread_stopped = true; }
void CV_Manager::ReleaseMats() { display_mat.release(); frame_gray.release(); detected_edges.release(); }

void CV_Manager::FlipCamera() {
    HaltCamera();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    m_selected_camera_type = (m_selected_camera_type == FRONT_CAMERA) ? BACK_CAMERA : FRONT_CAMERA;
    m_camera_thread_stopped = false;
    SetUpCamera();
    std::thread(&CV_Manager::CameraLoop, this).detach();
}
