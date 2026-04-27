//
// Created by ladiaviakoye on 02/03/2022.
//

#ifndef NATIVEBARCODETRACKER_ENCODER_H
#define NATIVEBARCODETRACKER_ENCODER_H

// local include
//#include "Camera.h"
// OpenCV biblio
#include "opencv2/core.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "socket_client-h264.h"
// Android biblio
#include <media/NdkMediaCodec.h>
#include <media/NdkMediaMuxer.h>
#include <media/NdkImage.h>
// Json biblio
//#include "cg-base/json.h"

// c biblio
#include <time.h>
#include <unistd.h>


class Encoder {
public:
    Encoder();

    virtual ~Encoder();

    void Encode (unsigned char *YUV_NV12, int planeSize);

    void InitCodec(int height , int width, int framerate ,int bitrate);

    media_status_t getStatus();

    void handleOutput();

    bool initFile(const char* H264_FILE_PATH, const char* JSON_FILE_PATH,
                  struct tm* timeInfos, time_t timestamp);

    bool writeFile(uint8_t* Buf, int32_t buffSize, size_t blocCount);

    bool json_close();

    bool json_init();

    bool json_addvalue(int tab);

    void setSocketClientH264(SocketClientH264 *m_ClientH264);

private:
    int mheight ;
    int mwidth ;
    int mYSize;


    uint8_t*       mHeaderBuf;
    int            mSizeofHeader;

    AMediaCodec*   mMediaCodec;
    media_status_t mStatus;
    AMediaFormat*  mMediaFormat;

    int            mRawH264Fd;
    pthread_t      mDecoderThread;

    SocketClientH264*      mClienth264;

    FILE*                 mfptr= NULL;
    FILE*                 mjptr= NULL;

    char     mfilename[255], mjsonfilename[255];
    //const char *pframe;

    /*cg_json_value_t *mroot_value ;
    cg_json_object_t *mroot_object ;

    cg_json_value_t *marray_value ;
    cg_json_array_t *marray_object ;*/

};

#endif //NATIVEBARCODETRACKER_ENCODER_H
