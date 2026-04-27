#ifndef ECPROJECT_NATIVE_CAMERA_H
#define ECPROJECT_NATIVE_CAMERA_H

#include <camera/NdkCameraManager.h>
#include "Util.h"

enum camera_type {
    BACK_CAMERA,
    FRONT_CAMERA,
};

class Native_Camera {
public:
    explicit Native_Camera(camera_type type);
    ~Native_Camera();

    bool MatchCaptureSizeRequest(ImageFormat *resView, int32_t width, int32_t height);
    bool CreateCaptureSession(ANativeWindow *window);
    int32_t GetOrientation();
    int32_t GetCameraCount();

private:
    ACameraManager *m_camera_manager;
    ACameraIdList *m_camera_id_list;
    const char *m_selected_camera_id;
    int32_t m_camera_orientation;
    ACameraDevice *m_camera_device;
    ACameraDevice_StateCallbacks m_device_state_callbacks;

    ACaptureRequest *m_capture_request;
    ACameraOutputTarget *m_camera_output_target;
    ACaptureSessionOutput *m_session_output;
    ACaptureSessionOutputContainer *m_capture_session_output_container;
    ACameraCaptureSession *m_capture_session;
    ACameraCaptureSession_stateCallbacks m_capture_session_state_callbacks;
    bool m_camera_ready;
};

#endif //ECPROJECT_NATIVE_CAMERA_H
