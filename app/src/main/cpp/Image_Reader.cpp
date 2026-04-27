// Image_Reader.cpp
// Correction : LOGE → LOGD pour le log de taille de buffer (non-erreur)

#include "Image_Reader.h"

#include <cstring>
#include <cstdlib>
#include "Util.h"

#define MAX_BUF_COUNT 2

void OnImageCallback(void *ctx, AImageReader *reader) {
    reinterpret_cast<Image_Reader *>(ctx)->ImageCallback(reader);
}

Image_Reader::Image_Reader(ImageFormat *res, enum AIMAGE_FORMATS format)
    : reader_(nullptr),
      presentRotation_(0),
      imageHeight_(res->height),
      imageWidth_(res->width) {

    media_status_t status = AImageReader_new(
        res->width, res->height, format, MAX_BUF_COUNT, &reader_);
    ASSERT(reader_ && status == AMEDIA_OK, "Failed to create AImageReader");

    AImageReader_ImageListener listener{
        .context         = this,
        .onImageAvailable = OnImageCallback,
    };
    AImageReader_setImageListener(reader_, &listener);

    // LOGD (pas LOGE) — c'est une info de debug, pas une erreur
    LOGD("[Image_Reader] Buffer alloc: %d bytes (%dx%d x4)",
         res->width * res->height * 4, res->width, res->height);
    imageBuffer_ = (uint8_t *) malloc(res->width * res->height * 4);
    ASSERT(imageBuffer_ != nullptr, "Failed to allocate imageBuffer_");
}

Image_Reader::~Image_Reader() {
    ASSERT(reader_, "NULL Pointer to %s", __FUNCTION__);
    AImageReader_delete(reader_);
    if (imageBuffer_) { free(imageBuffer_); imageBuffer_ = nullptr; }
}

void Image_Reader::ImageCallback(AImageReader *reader) {
    int32_t format;
    media_status_t status = AImageReader_getFormat(reader, &format);
    ASSERT(status == AMEDIA_OK, "Failed to get the media format");
    if (format == AIMAGE_FORMAT_JPEG) {
        AImage *image = nullptr;
        status = AImageReader_acquireNextImage(reader, &image);
        ASSERT(status == AMEDIA_OK && image, "Image is not available");
        int planeCount;
        status = AImage_getNumberOfPlanes(image, &planeCount);
        ASSERT(status == AMEDIA_OK && planeCount == 1,
               "Error: getNumberOfPlanes() planeCount = %d", planeCount);
        uint8_t *data = nullptr;
        int len = 0;
        AImage_getPlaneData(image, 0, &data, &len);
        AImage_delete(image);
    }
}

ANativeWindow *Image_Reader::GetNativeWindow(void) {
    if (!reader_) return nullptr;
    ANativeWindow *nativeWindow;
    media_status_t status = AImageReader_getWindow(reader_, &nativeWindow);
    ASSERT(status == AMEDIA_OK, "Could not get ANativeWindow");
    return nativeWindow;
}

AImage *Image_Reader::GetNextImage(void) {
    AImage *image;
    media_status_t status = AImageReader_acquireNextImage(reader_, &image);
    return (status == AMEDIA_OK) ? image : nullptr;
}

AImage *Image_Reader::GetLatestImage(void) {
    AImage *image;
    media_status_t status = AImageReader_acquireLatestImage(reader_, &image);
    return (status == AMEDIA_OK) ? image : nullptr;
}

int32_t Image_Reader::GetMaxImage(void) {
    int32_t image_count;
    media_status_t status = AImageReader_getMaxImages(reader_, &image_count);
    return (status == AMEDIA_OK) ? image_count : -1;
}

void Image_Reader::DeleteImage(AImage *image) {
    if (image) AImage_delete(image);
}

// ─── YUV → RGB ────────────────────────────────────────────────────────────────
#ifndef MAX
#define MAX(a, b) ({ __typeof__(a) _a=(a); __typeof__(b) _b=(b); _a>_b?_a:_b; })
#define MIN(a, b) ({ __typeof__(a) _a=(a); __typeof__(b) _b=(b); _a<_b?_a:_b; })
#endif

static const int kMaxChannelValue = 262143;

static inline uint32_t YUV2RGB(int nY, int nU, int nV) {
    nY -= 16; nU -= 128; nV -= 128;
    if (nY < 0) nY = 0;
    int nR = (int)(1192 * nY + 1634 * nV);
    int nG = (int)(1192 * nY -  833 * nV - 400 * nU);
    int nB = (int)(1192 * nY + 2066 * nU);
    nR = MIN(kMaxChannelValue, MAX(0, nR));
    nG = MIN(kMaxChannelValue, MAX(0, nG));
    nB = MIN(kMaxChannelValue, MAX(0, nB));
    return 0xff000000 | ((nR >> 10) << 16) | ((nG >> 10) << 8) | (nB >> 10);
}

bool Image_Reader::DisplayImage(ANativeWindow_Buffer *buf, AImage *image) {
    ASSERT(buf->format == WINDOW_FORMAT_RGBX_8888 ||
           buf->format == WINDOW_FORMAT_RGBA_8888,
           "Not supported buffer format");

    int32_t srcFormat = -1;
    AImage_getFormat(image, &srcFormat);
    ASSERT(AIMAGE_FORMAT_YUV_420_888 == srcFormat, "Failed to get format");

    int32_t srcPlanes = 0;
    AImage_getNumberOfPlanes(image, &srcPlanes);
    ASSERT(srcPlanes == 3, "Is not 3 planes");

    ConvertYUV420ToRGBA(image, rgbaScratch_);

    cv::Mat *oriented = &rgbaScratch_;
    if (presentRotation_ != 0) {
        if (rgbaRotScratch_.empty() ||
            rgbaRotScratch_.cols != rgbaScratch_.rows ||
            rgbaRotScratch_.rows != rgbaScratch_.cols) {
            rgbaRotScratch_ = cv::Mat(rgbaScratch_.cols, rgbaScratch_.rows, CV_8UC4);
        }
        switch (presentRotation_) {
            case 90:  cv::rotate(rgbaScratch_, rgbaRotScratch_, cv::ROTATE_90_CLOCKWISE);        oriented = &rgbaRotScratch_; break;
            case 180: cv::rotate(rgbaScratch_, rgbaRotScratch_, cv::ROTATE_180);                 oriented = &rgbaRotScratch_; break;
            case 270: cv::rotate(rgbaScratch_, rgbaRotScratch_, cv::ROTATE_90_COUNTERCLOCKWISE); oriented = &rgbaRotScratch_; break;
            default:  ASSERT(0, "NOT recognized display rotation: %d", presentRotation_);
        }
    }

    if (!loggedOnce_) {
        LOGI("[Image_Reader] Buffer: %dx%d stride=%d fmt=%d | Src(after rot): %dx%d rot=%d",
             buf->width, buf->height, buf->stride, buf->format,
             oriented->cols, oriented->rows, presentRotation_);
        loggedOnce_ = true;
    }

    BlitCenterCropToBuffer(buf, *oriented);
    AImage_delete(image);
    return true;
}

void Image_Reader::SetPresentRotation(int32_t angle) { presentRotation_ = angle; }

void Image_Reader::ConvertYUV420ToRGBA(AImage *image, cv::Mat &outRGBA) {
    int32_t width, height;
    AImage_getWidth(image, &width);
    AImage_getHeight(image, &height);

    if (outRGBA.empty() || outRGBA.cols != width || outRGBA.rows != height)
        outRGBA = cv::Mat(height, width, CV_8UC4);

    uint8_t *yData = nullptr, *uData = nullptr, *vData = nullptr;
    int yLen = 0, uLen = 0, vLen = 0;
    int yStride = 0, uvStride = 0, uvPixelStride = 0;

    AImage_getPlaneData(image, 0, &yData, &yLen);
    AImage_getPlaneData(image, 1, &uData, &uLen);
    AImage_getPlaneData(image, 2, &vData, &vLen);
    AImage_getPlaneRowStride(image, 0, &yStride);
    AImage_getPlaneRowStride(image, 1, &uvStride);
    AImage_getPlanePixelStride(image, 1, &uvPixelStride);

    for (int y = 0; y < height; y++) {
        const uint8_t *pY  = yData + yStride * y;
        int uvRow          = uvStride * (y >> 1);
        const uint8_t *pU  = uData + uvRow;
        const uint8_t *pV  = vData + uvRow;
        uint32_t *outRow   = outRGBA.ptr<uint32_t>(y);
        for (int x = 0; x < width; x++) {
            int uvOffset = (x >> 1) * uvPixelStride;
            outRow[x] = YUV2RGB(pY[x], pU[uvOffset], pV[uvOffset]);
        }
    }
}

void Image_Reader::BlitCenterCropToBuffer(ANativeWindow_Buffer *buf, const cv::Mat &srcRGBA) {
    const int dstW = buf->width;
    const int dstH = buf->height;
    const int srcW = srcRGBA.cols;
    const int srcH = srcRGBA.rows;

    const float scaleX = (float)srcW / dstW;
    const float scaleY = (float)srcH / dstH;
    const float scale  = std::max(scaleX, scaleY);
    const int cropW    = (int)(dstW * scale);
    const int cropH    = (int)(dstH * scale);
    const int offX     = (srcW - cropW) / 2;
    const int offY     = (srcH - cropH) / 2;

    cv::Rect roi(offX, offY,
                 std::min(cropW, srcW - offX),
                 std::min(cropH, srcH - offY));
    cv::Mat cropped = srcRGBA(roi);
    cv::Mat dst(dstH, dstW, CV_8UC4, buf->bits, buf->stride * 4);
    cv::resize(cropped, dst, dst.size(), 0, 0, cv::INTER_LINEAR);
}
