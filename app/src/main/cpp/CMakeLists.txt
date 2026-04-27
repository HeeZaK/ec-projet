cmake_minimum_required(VERSION 3.22.1)
project("ECproject")

# ─── OpenCV ────────────────────────────────────────────────────────────────────
# cmake -DOpenCV_DIR=/chemin/vers/OpenCV-android-sdk/sdk/native/jni
# ou variable d'env OPENCV_ANDROID_SDK_PATH
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

# ─── IXWebSocket ───────────────────────────────────────────────────────────────
# git submodule add https://github.com/machinezone/IXWebSocket.git \
#   app/src/main/cpp/third_party/IXWebSocket
# Désactiver TLS si pas de BoringSSL séparé (libwebrtc inclut déjà BoringSSL)
set(USE_TLS             OFF CACHE BOOL "" FORCE)
set(USE_OPEN_SSL        OFF CACHE BOOL "" FORCE)
set(IXWEBSOCKET_INSTALL OFF CACHE BOOL "" FORCE)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/IXWebSocket/CMakeLists.txt")
    add_subdirectory(third_party/IXWebSocket)
    set(IXWEBSOCKET_AVAILABLE TRUE)
    message(STATUS "[ECproject] IXWebSocket trouvé")
else()
    message(WARNING
        "[ECproject] IXWebSocket introuvable.\n"
        "Exécutez : git submodule add https://github.com/machinezone/IXWebSocket.git "
        "app/src/main/cpp/third_party/IXWebSocket")
    set(IXWEBSOCKET_AVAILABLE FALSE)
endif()

# ─── libwebrtc ─────────────────────────────────────────────────────────────────
# Télécharger le prebuild : https://github.com/crow-misia/libwebrtc-android
# Placer les .so dans app/src/main/jniLibs/<ABI>/
# et les headers dans app/src/main/cpp/third_party/libwebrtc/include/
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/libwebrtc/include/api/peer_connection_interface.h")
    set(LIBWEBRTC_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/third_party/libwebrtc/include")
    set(LIBWEBRTC_SO      "${CMAKE_CURRENT_SOURCE_DIR}/third_party/libwebrtc/lib/${ANDROID_ABI}/libwebrtc.so")
    add_library(webrtc SHARED IMPORTED)
    set_target_properties(webrtc PROPERTIES IMPORTED_LOCATION ${LIBWEBRTC_SO})
    set(LIBWEBRTC_AVAILABLE TRUE)
    message(STATUS "[ECproject] libwebrtc trouvé")
else()
    message(WARNING
        "[ECproject] libwebrtc introuvable — WebRtcEngine compilera en mode stub.\n"
        "Voir : https://github.com/crow-misia/libwebrtc-android")
    set(LIBWEBRTC_AVAILABLE FALSE)
endif()

# ─── Sources ───────────────────────────────────────────────────────────────────
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
    signaling/SdpSerializer.cpp
    signaling/JsonProtocol.cpp
    # Security
    security/AuthManager.cpp
    # WebRTC
    webrtc/WebRtcEngine.cpp
    webrtc/VideoTrackSource.cpp
    webrtc/AudioController.cpp
    webrtc/DataChannelController.cpp
    webrtc/PeerObserver.cpp
)

# ─── Includes ──────────────────────────────────────────────────────────────────
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/telemetry
    ${CMAKE_CURRENT_SOURCE_DIR}/signaling
    ${CMAKE_CURRENT_SOURCE_DIR}/security
    ${CMAKE_CURRENT_SOURCE_DIR}/webrtc
    ${OpenCV_INCLUDE_DIRS}
)

if(IXWEBSOCKET_AVAILABLE)
    target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/IXWebSocket
    )
endif()

if(LIBWEBRTC_AVAILABLE)
    target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
        ${LIBWEBRTC_INCLUDE}
    )
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE WEBRTC_ANDROID WEBRTC_POSIX)
endif()

# ─── Compile flags ─────────────────────────────────────────────────────────────
target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE
    -Wall -Wextra
    -fexceptions
    -frtti
)

# ─── Links ─────────────────────────────────────────────────────────────────────
target_link_libraries(${CMAKE_PROJECT_NAME}
    ${OpenCV_LIBS}
    camera2ndk
    mediandk
    android
    log
    OpenSLES
)

if(IXWEBSOCKET_AVAILABLE)
    target_link_libraries(${CMAKE_PROJECT_NAME} ixwebsocket)
endif()

if(LIBWEBRTC_AVAILABLE)
    target_link_libraries(${CMAKE_PROJECT_NAME} webrtc)
endif()
