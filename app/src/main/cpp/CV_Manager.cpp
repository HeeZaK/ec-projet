// CV_Manager.cpp
// Corrections :
//   - suppression des 'using namespace std/cv' globaux (restent locaux aux fonctions si nécessaire)
//   - IP déplacée vers AppConfig
//   - busy-wait remplacé par usleep(1000) déjà présent → conservé
//   - display_mat protégé par m_mat_mutex
//   - FlipCamera() utilise TearDownCamera() + join propre (via StopCameraLoop)

#include "CV_Manager.h"

CV_Manager::CV_Manager()
    : m_camera_ready(false), m_image(nullptr),
      m_image_reader(nullptr), m_native_camera(nullptr) {}

CV_Manager::~CV_Manager() {
    if (m_native_camera) { delete m_native_camera; m_native_camera = nullptr; }
    if (m_native_window) { ANativeWindow_release(m_native_window); m_native_window = nullptr; }
    if (m_image_reader)  { delete m_image_reader;  m_image_reader  = nullptr; }
}

void CV_Manager::SetNativeWindow(ANativeWindow *native_window) {
    m_native_window = native_window;
}

void CV_Manager::SetUpCamera() {
    m_native_camera = new Native_Camera(m_selected_camera_type);

    const int nativeWidth  = ANativeWindow_getWidth(m_native_window);
    const int nativeHeight = ANativeWindow_getHeight(m_native_window);

    LOGI("[CV_Manager] Native window: %dx%d", nativeWidth, nativeHeight);
    ASSERT(nativeWidth > 0 && nativeHeight > 0, "[CV_Manager] Invalid native window size");

    m_native_camera->MatchCaptureSizeRequest(&m_view, nativeWidth, nativeHeight);
    ASSERT(m_view.width && m_view.height, "[CV_Manager] Could not find supportable capture resolution");

    LOGI("[CV_Manager] Capture size (YUV): %dx%d format=%d", m_view.width, m_view.height, m_view.format);

    ANativeWindow_setBuffersGeometry(
        m_native_window, nativeWidth, nativeHeight, WINDOW_FORMAT_RGBX_8888);

    m_image_reader = new Image_Reader(&m_view, AIMAGE_FORMAT_YUV_420_888);

    const int orientation = m_native_camera->GetOrientation();
    m_image_reader->SetPresentRotation(orientation);
    LOGI("[CV_Manager] Present rotation: %d", orientation);

    ANativeWindow *image_reader_window = m_image_reader->GetNativeWindow();
    m_camera_ready = m_native_camera->CreateCaptureSession(image_reader_window);
    LOGI("[CV_Manager] Camera session ready: %s", m_camera_ready ? "true" : "false");
}

void CV_Manager::SetUpTCP() {
    // L'IP et le port sont lus depuis AppConfig — jamais hardcodés ici.
    m_encoder     = new Encoder();
    m_socket_h264 = new SocketClientH264(AppConfig::SERVER_IP, AppConfig::SERVER_PORT);
}

void CV_Manager::StartCameraLoop() {
    m_camera_thread_stopped = false;
    if (m_camera_thread.joinable()) {
        m_camera_thread.join();
    }
    m_camera_thread = std::thread(&CV_Manager::CameraLoop, this);
}

void CV_Manager::StopCameraLoop() {
    m_camera_thread_stopped = true;
    m_camera_ready          = false;
    if (m_camera_thread.joinable()) {
        m_camera_thread.join();
    }
}

void CV_Manager::TearDownCamera() {
    StopCameraLoop();
    if (m_image_reader)  { delete m_image_reader;  m_image_reader  = nullptr; }
    if (m_native_camera) { delete m_native_camera; m_native_camera = nullptr; }
}

void CV_Manager::CameraLoop() {
    bool buffer_printout = false;

    while (!m_camera_thread_stopped) {
        if (!m_camera_ready || m_image_reader == nullptr || m_native_window == nullptr) {
            usleep(1000);
            continue;
        }

        AImage *image = m_image_reader->GetLatestImage();
        if (image == nullptr) {
            usleep(1000);
            continue;
        }

        ANativeWindow_acquire(m_native_window);

        ANativeWindow_Buffer buffer;
        if (ANativeWindow_lock(m_native_window, &buffer, nullptr) < 0) {
            ANativeWindow_release(m_native_window);
            m_image_reader->DeleteImage(image);
            continue;
        }

        if (!buffer_printout) {
            buffer_printout = true;
            LOGI("/// H-W-S-F: %d, %d, %d, %d",
                 buffer.height, buffer.width, buffer.stride, buffer.format);
        }

        m_image_reader->DisplayImage(&buffer, image);

        {
            // Accès à display_mat protégé
            std::lock_guard<std::mutex> lk(m_mat_mutex);
            display_mat = cv::Mat(
                buffer.height, buffer.width,
                CV_8UC4, buffer.bits, buffer.stride * 4);
        }

        ANativeWindow_unlockAndPost(m_native_window);
        ANativeWindow_release(m_native_window);

        ReleaseMats();
    }

    LOGI("CameraLoop stopped cleanly");
}

bool CV_Manager::IsInitialized() const  { return m_initialized; }
void CV_Manager::SetInitialized(bool v)  { m_initialized = v; }

void CV_Manager::RunCV() {
    scan_mode = true;
    total_t   = 0;
    start_t   = clock();
}

void CV_Manager::HaltCamera() {
    if (!m_native_camera) {
        LOGE("HaltCamera: pas de caméra initialisée");
        return;
    }
    if (m_native_camera->GetCameraCount() < 2) {
        LOGE("HaltCamera: une seule caméra disponible");
        return;
    }
    m_camera_thread_stopped = true;
}

void CV_Manager::FlipCamera() {
    // FlipCamera est appelé depuis le thread UI.
    // TearDownCamera() fait un join() propre du thread caméra avant de
    // détruire les ressources — pas de heuristique sleep.
    std::lock_guard<std::mutex> lock(m_camera_mutex);

    if (!m_native_camera) {
        LOGE("FlipCamera: caméra non initialisée");
        return;
    }
    if (m_native_camera->GetCameraCount() < 2) {
        LOGE("FlipCamera: une seule caméra disponible");
        return;
    }

    LOGI("FlipCamera: arrêt de la caméra courante");
    TearDownCamera();

    m_selected_camera_type =
        (m_selected_camera_type == FRONT_CAMERA) ? BACK_CAMERA : FRONT_CAMERA;

    LOGI("FlipCamera: démarrage de la nouvelle caméra");
    SetUpCamera();
    StartCameraLoop();
}

void CV_Manager::ReleaseMats() {
    std::lock_guard<std::mutex> lk(m_mat_mutex);
    display_mat.release();
    frame_gray.release();
    grad_x.release();     abs_grad_x.release();
    grad_y.release();     abs_grad_y.release();
    detected_edges.release();
    thresh.release();
    kernel.release();
    cleaned.release();
    hierarchy.clear();
}
