cmake_minimum_required(VERSION 3.22.1)
project("ECproject")

# ─── OpenCV ───────────────────────────────────────────────────────────────────
# Passer le chemin via : cmake -DOpenCV_DIR=/path/to/OpenCV-android-sdk/sdk/native/jni
# Ou définir la variable d'environnement OPENCV_ANDROID_SDK_PATH
if(NOT DEFINED OpenCV_DIR)
    if(DEFINED ENV{OPENCV_ANDROID_SDK_PATH})
        set(OpenCV_DIR "$ENV{OPENCV_ANDROID_SDK_PATH}/sdk/native/jni")
    else()
        message(FATAL_ERROR
            "OpenCV_DIR n'est pas défini.\n"
            "Passez -DOpenCV_DIR=/chemin/vers/OpenCV-android-sdk/sdk/native/jni"
            " ou définissez la variable d'env OPENCV_ANDROID_SDK_PATH.")
    endif()
endif()
find_package(OpenCV REQUIRED)

# ─── Sources ──────────────────────────────────────────────────────────────────
add_library(${CMAKE_PROJECT_NAME} SHARED
    native-lib.cpp
    CV_Manager.cpp
    Native_Camera.cpp
    Image_Reader.cpp
    Encoder.cpp
    socket_client-h264.cpp
    # Core
    core/DeviceRuntime.cpp
    # Telemetry
    telemetry/DeviceState.cpp
    telemetry/TelemetryPublisher.cpp
    # Signaling
    signaling/SignalingClient.cpp
    # Security
    security/AuthManager.cpp
    # WebRTC stub
    webrtc/WebRtcEngine.cpp
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/telemetry
    ${CMAKE_CURRENT_SOURCE_DIR}/signaling
    ${CMAKE_CURRENT_SOURCE_DIR}/security
    ${CMAKE_CURRENT_SOURCE_DIR}/webrtc
    ${OpenCV_INCLUDE_DIRS}
)

# ─── Links ────────────────────────────────────────────────────────────────────
target_link_libraries(${CMAKE_PROJECT_NAME}
    ${OpenCV_LIBS}
    camera2ndk
    mediandk
    android
    log
)
