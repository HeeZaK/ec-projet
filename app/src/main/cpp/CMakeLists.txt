cmake_minimum_required(VERSION 3.22.1)
project("ECproject")

# Configuration OpenCV
Set(OpenCV_DIR "/Users/benjos/AndroidStudioProjects/OpenCV-android-sdk/sdk/native/jni")
find_package(OpenCV REQUIRED)

# Liste des fichiers sources
add_library(${CMAKE_PROJECT_NAME} SHARED
    native-lib.cpp
    CV_Manager.cpp
    Native_Camera.cpp
    Image_Reader.cpp
    Encoder.cpp
    socket_client-h264.cpp
)

# Liens vers les bibliothèques système et externes
target_link_libraries(${CMAKE_PROJECT_NAME}
    ${OpenCV_LIBS}
    camera2ndk
    mediandk
    android
    log
)
