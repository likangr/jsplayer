#ifndef JS_MEDIACODEC_DEF
#define JS_MEDIACODEC_DEF

#include "libyuv.h"

#ifdef  __cplusplus
extern "C" {
#include "libavcodec/avcodec.h"
using namespace libyuv;
#else

#include "libavcodec/avcodec.h"

#endif

/**
* Enumeration defining possible uncompressed image/video formats.
*
* ENUMS:
*  Unused                 : Placeholder value when format is N/A
*  Monochrome             : black and white
*  8bitRGB332             : Red 7:5, Green 4:2, Blue 1:0
*  12bitRGB444            : Red 11:8, Green 7:4, Blue 3:0
*  16bitARGB4444          : Alpha 15:12, Red 11:8, Green 7:4, Blue 3:0
*  16bitARGB1555          : Alpha 15, Red 14:10, Green 9:5, Blue 4:0
*  16bitRGB565            : Red 15:11, Green 10:5, Blue 4:0
*  16bitBGR565            : Blue 15:11, Green 10:5, Red 4:0
*  18bitRGB666            : Red 17:12, Green 11:6, Blue 5:0
*  18bitARGB1665          : Alpha 17, Red 16:11, Green 10:5, Blue 4:0
*  19bitARGB1666          : Alpha 18, Red 17:12, Green 11:6, Blue 5:0
*  24bitRGB888            : Red 24:16, Green 15:8, Blue 7:0
*  24bitBGR888            : Blue 24:16, Green 15:8, Red 7:0
*  24bitARGB1887          : Alpha 23, Red 22:15, Green 14:7, Blue 6:0
*  25bitARGB1888          : Alpha 24, Red 23:16, Green 15:8, Blue 7:0
*  32bitBGRA8888          : Blue 31:24, Green 23:16, Red 15:8, Alpha 7:0
*  32bitARGB8888          : Alpha 31:24, Red 23:16, Green 15:8, Blue 7:0
*  YUV411Planar           : U,Y are subsampled by a factor of 4 horizontally
*  YUV411PackedPlanar     : packed per payload in planar slices
*  YUV420Planar           : Three arrays Y,U,V.
*  YUV420PackedPlanar     : packed per payload in planar slices
*  YUV420SemiPlanar       : Two arrays, one is all Y, the other is U and V
*  YUV422Planar           : Three arrays Y,U,V.
*  YUV422PackedPlanar     : packed per payload in planar slices
*  YUV422SemiPlanar       : Two arrays, one is all Y, the other is U and V
*  YCbYCr                 : Organized as 16bit YUYV (i.e. YCbYCr)
*  YCrYCb                 : Organized as 16bit YVYU (i.e. YCrYCb)
*  CbYCrY                 : Organized as 16bit UYVY (i.e. CbYCrY)
*  CrYCbY                 : Organized as 16bit VYUY (i.e. CrYCbY)
*  YUV444Interleaved      : Each pixel contains equal parts YUV
*  RawBayer8bit           : SMIA camera output format
*  RawBayer10bit          : SMIA camera output format
*  RawBayer8bitcompressed : SMIA camera output format
*/
//typedef enum OMX_COLOR_FORMATTYPE {
//    OMX_COLOR_FormatUnused,
//    OMX_COLOR_FormatMonochrome,
//    OMX_COLOR_Format8bitRGB332,
//    OMX_COLOR_Format12bitRGB444,
//    OMX_COLOR_Format16bitARGB4444,
//    OMX_COLOR_Format16bitARGB1555,
//    OMX_COLOR_Format16bitRGB565,
//    OMX_COLOR_Format16bitBGR565,
//    OMX_COLOR_Format18bitRGB666,
//    OMX_COLOR_Format18bitARGB1665,
//    OMX_COLOR_Format19bitARGB1666,
//    OMX_COLOR_Format24bitRGB888,
//    OMX_COLOR_Format24bitBGR888,
//    OMX_COLOR_Format24bitARGB1887,
//    OMX_COLOR_Format25bitARGB1888,
//    OMX_COLOR_Format32bitBGRA8888,
//    OMX_COLOR_Format32bitARGB8888,
//    OMX_COLOR_FormatYUV411Planar,
//    OMX_COLOR_FormatYUV411PackedPlanar,
//    OMX_COLOR_FormatYUV420Planar,
//    OMX_COLOR_FormatYUV420PackedPlanar,
//    OMX_COLOR_FormatYUV420SemiPlanar,
//    OMX_COLOR_FormatYUV422Planar,
//    OMX_COLOR_FormatYUV422PackedPlanar,
//    OMX_COLOR_FormatYUV422SemiPlanar,
//    OMX_COLOR_FormatYCbYCr,
//    OMX_COLOR_FormatYCrYCb,
//    OMX_COLOR_FormatCbYCrY,
//    OMX_COLOR_FormatCrYCbY,
//    OMX_COLOR_FormatYUV444Interleaved,
//    OMX_COLOR_FormatRawBayer8bit,
//    OMX_COLOR_FormatRawBayer10bit,
//    OMX_COLOR_FormatRawBayer8bitcompressed,
//    OMX_COLOR_FormatL2,
//    OMX_COLOR_FormatL4,
//    OMX_COLOR_FormatL8,
//    OMX_COLOR_FormatL16,
//    OMX_COLOR_FormatL24,
//    OMX_COLOR_FormatL32,
//    OMX_COLOR_FormatYUV420PackedSemiPlanar,
//    OMX_COLOR_FormatYUV422PackedSemiPlanar,
//    OMX_COLOR_Format18BitBGR666,
//    OMX_COLOR_Format24BitARGB6666,
//    OMX_COLOR_Format24BitABGR6666,
//    OMX_COLOR_FormatKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */
//    OMX_COLOR_FormatVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
//    OMX_COLOR_TI_FormatYUV420PackedSemiPlanar = 0x7f000100,
//    OMX_COLOR_QCOM_FormatYUV420SemiPlanar = 0x7fa30c00,
//    OMX_COLOR_FormatMax = 0x7FFFFFFF
//} OMX_COLOR_FORMATTYPE;


#define  JS_MEDIAFORMAT_KEY_AAC_PROFILE  "aac-profile"
#define JS_MEDIAFORMAT_KEY_BIT_RATE  "bitrate"
#define JS_MEDIAFORMAT_KEY_SMPLE_RATE  "sample-rate"
#define JS_MEDIAFORMAT_KEY_CHANNEL_COUNT  "channel-count"
#define JS_MEDIAFORMAT_KEY_CHANNEL_MASK  "channel-mask"
#define JS_MEDIAFORMAT_KEY_DURATION  "durationUs"
#define JS_MEDIAFORMAT_KEY_FLAC_COMPRESSION_LEVEL  "flac-compression-level"
#define JS_MEDIAFORMAT_KEY_IS_ADTS  "is-adts"
#define JS_MEDIAFORMAT_KEY_IS_AUTOSELECT  "is-autoselect"
#define JS_MEDIAFORMAT_KEY_IS_DEFAULT  "is-default"
#define JS_MEDIAFORMAT_KEY_IS_FORCED_SUBTITLE  "is-forced-subtitle"
#define JS_MEDIAFORMAT_KEY_I_FRME_INTERVAL  "i-frame-interval"
#define JS_MEDIAFORMAT_KEY_LANGUAGE  "language"
#define JS_MEDIAFORMAT_KEY_MAX_HEIGHT  "max-height"
#define JS_MEDIAFORMAT_KEY_MAX_WIDTH  "max-width"
#define JS_MEDIAFORMAT_KEY_MAX_INPUT_SIZE  "max-input-size"
#define JS_MEDIAFORMAT_KEY_HEIGHT  "height"
#define JS_MEDIAFORMAT_KEY_WIDTH  "width"
#define JS_MEDIAFORMAT_KEY_STRIDE  "stride"
#define JS_MEDIAFORMAT_KEY_COLOR_FORMAT  "color-format"
#define JS_MEDIAFORMAT_KEY_SLICE_HEIGHT  "slice-height"
#define JS_MEDIAFORMAT_KEY_CROP_LEFT  "crop-left"
#define JS_MEDIAFORMAT_KEY_CROP_RIGHT  "crop-right"
#define JS_MEDIAFORMAT_KEY_CROP_TOP  "crop-top"
#define JS_MEDIAFORMAT_KEY_CROP_BOTTOM  "crop-bottom"
#define JS_MEDIAFORMAT_KEY_FRME_RATE  "frame-rate"
#define JS_MEDIAFORMAT_KEY_MIME  "mime"
#define JS_MEDIAFORMAT_KEY_PUSH_BLANK_BUFFERS_ON_STOP  "push-blank-buffers-on-destroy"
#define JS_MEDIAFORMAT_KEY_REPEAT_PREVIOUS_FRME_AFTER  "repeat-previous-frame-after"


#define JS_MIME_VIDEO_VP8 "video/x-vnd.on2.vp8"   //- VP8 video (i.e. video in .webm)
#define JS_MIME_VIDEO_VP9         "video/x-vnd.on2.vp9"   //- VP9 video (i.e. video in .webm)
#define JS_MIME_VIDEO_AVC         "video/avc"             //- H.264/AVC video
#define JS_MIME_VIDEO_HEVC        "video/hevc"            //- H.265/HEVC video
#define JS_MIME_VIDEO_MPEG2VIDEO  "video/mpeg2"           //- MPEG2 video
#define JS_MIME_VIDEO_MPEG4       "video/mp4v-es"         //- MPEG4 video
#define JS_MIME_VIDEO_H263        "video/3gpp"            //- H.263 video
#define JS_MIME_AUDIO_AMR_NB      "audio/3gpp"            //- AMR narrowband audio
#define JS_MIME_AUDIO_AMR_WB      "audio/amr-wb"          //- AMR wideband audio
#define JS_MIME_AUDIO_MP3         "audio/mpeg"            //- MPEG1/2 audio layer III
#define JS_MIME_AUDIO_RAW_AAC     "audio/mp4a-latm"       //- AAC audio (note, this is raw AAC packets, not packaged in LATM!)
#define JS_MIME_AUDIO_VORBIS      "audio/vorbis"          //- vorbis audio
#define JS_MIME_AUDIO_G711_ALAW   "audio/g711-alaw"       //- G.711 alaw audio
#define JS_MIME_AUDIO_G711_MLAW   "audio/g711-mlaw"       //- G.711 ulaw audio


enum {
    OMX_COLOR_FormatMonochrome = 1,
    OMX_COLOR_Format8bitRGB332 = 2,
    OMX_COLOR_Format12bitRGB444 = 3,
    OMX_COLOR_Format16bitARGB4444 = 4,
    OMX_COLOR_Format16bitARGB1555 = 5,
    OMX_COLOR_Format16bitRGB565 = 6,
    OMX_COLOR_Format16bitBGR565 = 7,
    OMX_COLOR_Format18bitRGB666 = 8,
    OMX_COLOR_Format18bitARGB1665 = 9,
    OMX_COLOR_Format19bitARGB1666 = 10,
    OMX_COLOR_Format24bitRGB888 = 11,
    OMX_COLOR_Format24bitBGR888 = 12,
    OMX_COLOR_Format24bitARGB1887 = 13,
    OMX_COLOR_Format25bitARGB1888 = 14,
    OMX_COLOR_Format32bitBGRA8888 = 15,
    OMX_COLOR_Format32bitARGB8888 = 16,
    OMX_COLOR_FormatYUV411Planar = 17,
    OMX_COLOR_FormatYUV411PackedPlanar = 18,
    OMX_COLOR_FormatYUV420Planar = 19,
    OMX_COLOR_FormatYUV420PackedPlanar = 20,
    OMX_COLOR_FormatYUV420SemiPlanar = 21,
    OMX_COLOR_FormatYUV422Planar = 22,
    OMX_COLOR_FormatYUV422PackedPlanar = 23,
    OMX_COLOR_FormatYUV422SemiPlanar = 24,
    OMX_COLOR_FormatYCbYCr = 25,
    OMX_COLOR_FormatYCrYCb = 26,
    OMX_COLOR_FormatCbYCrY = 27,
    OMX_COLOR_FormatCrYCbY = 28,
    OMX_COLOR_FormatYUV444Interleaved = 29,
    OMX_COLOR_FormatRawBayer8bit = 30,
    OMX_COLOR_FormatRawBayer10bit = 31,
    OMX_COLOR_FormatRawBayer8bitcompressed = 32,
    OMX_COLOR_FormatL2 = 33,
    OMX_COLOR_FormatL4 = 34,
    OMX_COLOR_FormatL8 = 35,
    OMX_COLOR_FormatL16 = 36,
    OMX_COLOR_FormatL24 = 37,
    OMX_COLOR_FormatL32 = 38,
    OMX_COLOR_FormatYUV420PackedSemiPlanar = 39,
    OMX_COLOR_FormatYUV422PackedSemiPlanar = 40,
    OMX_COLOR_Format18BitBGR666 = 41,
    OMX_COLOR_Format24BitARGB6666 = 42,
    OMX_COLOR_Format24BitABGR6666 = 43,

    // OMX_COLOR_TI_FormatYUV420PackedSemiPlanar = 0x7f000100,
            OMX_COLOR_FormatSurface = 0x7f000789,
    OMX_COLOR_FormatYUV420Flexible = 0x7f420888,
    //  OMX_COLOR_QCOM_FormatYUV420SemiPlanar = 0x7fa30c00,

    // from hardware intel
            OMX_INTEL_COLOR_FormatYUV420PackedSemiPlanar = 0x7FA00E00,
    OMX_INTEL_COLOR_FormatYUV420PackedSemiPlanar_Tiled = 0x7FA00F00,

    // from hardware qcom
            OMX_QCOM_COLOR_FormatYVU420SemiPlanar = 0x7FA30C00,
    OMX_QCOM_COLOR_FormatYVU420PackedSemiPlanar32m4ka = 0x7FA30C01,
    OMX_QCOM_COLOR_FormatYUV420PackedSemiPlanar16m2ka = 0x7FA30C02,
    OMX_QCOM_COLOR_FormatYUV420PackedSemiPlanar64x32Tile2m8ka = 0x7FA30C03,
    OMX_QCOM_COLOR_FORMATYUV420PackedSemiPlanar32m = 0x7FA30C04,
    OMX_QCOM_COLOR_FORMATYUV420PackedSemiPlanar32mMultiView = 0x7FA30C05,

    // from hardware samsung_slsi
            OMX_SEC_COLOR_FormatNV12TPhysicalAddress = 0x7F000001,
    OMX_SEC_COLOR_FormatNV12LPhysicalAddress = 0x7F000002,
    OMX_SEC_COLOR_FormatNV12LVirtualAddress = 0x7F000003,
    OMX_SEC_COLOR_FormatNV12Tiled = 0x7FC00002,
    OMX_SEC_COLOR_FormatNV21LPhysicalAddress = 0x7F000010,
    OMX_SEC_COLOR_FormatNV21Linear = 0x7F000011,

    // from hardware ti
            OMX_TI_COLOR_FormatRawBayer10bitStereo = 0x7F000002,
    OMX_TI_COLOR_FormatYUV420PackedSemiPlanar = 0x7F000100,
};


static const struct {

    int color_format;
    enum AVPixelFormat pix_fmt;
    uint32_t yuv_fourcc;

} color_formats[] = {
        {OMX_COLOR_FormatYUV411Planar,                              AV_PIX_FMT_YUV411P,   FOURCC_I411},
        {OMX_COLOR_FormatYUV411PackedPlanar,                        AV_PIX_FMT_UYYVYY411, FOURCC_I411},//
        {OMX_COLOR_FormatYUV420Planar,                              AV_PIX_FMT_YUV420P,   FOURCC_I420},
        {OMX_COLOR_FormatYUV420PackedPlanar,                        AV_PIX_FMT_NV12,      FOURCC_NV12},//
        {OMX_COLOR_FormatYUV420SemiPlanar,                          AV_PIX_FMT_NV12,      FOURCC_NV12},

        {OMX_COLOR_FormatYUV422Planar,                              AV_PIX_FMT_YUV422P,   FOURCC_I422},
        {OMX_COLOR_FormatYUV422PackedPlanar,                        AV_PIX_FMT_YVYU422,   FOURCC_I422},
        {OMX_COLOR_FormatYUV422SemiPlanar,                          AV_PIX_FMT_YVYU422,   FOURCC_I422},//
        {OMX_COLOR_FormatYCbYCr,                                    AV_PIX_FMT_NV12,      FOURCC_NV12},//
        {OMX_COLOR_FormatYCrYCb,                                    AV_PIX_FMT_NV12,      FOURCC_NV12},//
        {OMX_COLOR_FormatCbYCrY,                                    AV_PIX_FMT_NV12,      FOURCC_NV12},//
        {OMX_COLOR_FormatCrYCbY,                                    AV_PIX_FMT_NV12,      FOURCC_NV12},//
        {OMX_COLOR_FormatYUV444Interleaved,                         AV_PIX_FMT_YUV444P,   FOURCC_I444},//
//        {OMX_COLOR_FormatRawBayer8bit,                              AV_PIX_FMT_UYYVYY411},
//        {OMX_COLOR_FormatRawBayer10bit,                             AV_PIX_FMT_UYYVYY411},
//        {OMX_COLOR_FormatRawBayer8bitcompressed,                    AV_PIX_FMT_UYYVYY411},
//        {OMX_COLOR_FormatL2,                                        AV_PIX_FMT_UYYVYY411},
//        {OMX_COLOR_FormatL4,                                        AV_PIX_FMT_UYYVYY411},
//        {OMX_COLOR_FormatL8,                                        AV_PIX_FMT_UYYVYY411},
//        {OMX_COLOR_FormatL16,                                       AV_PIX_FMT_UYYVYY411},
//        {OMX_COLOR_FormatL24,                                       AV_PIX_FMT_UYYVYY411},
//        {OMX_COLOR_FormatL32,                                       AV_PIX_FMT_UYYVYY411},
        {OMX_COLOR_FormatYUV420PackedSemiPlanar,                    AV_PIX_FMT_NV12,      FOURCC_NV12},
        {OMX_COLOR_FormatYUV422PackedSemiPlanar,                    AV_PIX_FMT_YVYU422,   FOURCC_I422},//
        {OMX_INTEL_COLOR_FormatYUV420PackedSemiPlanar,              AV_PIX_FMT_NV12,      FOURCC_NV12},
        {OMX_INTEL_COLOR_FormatYUV420PackedSemiPlanar_Tiled,        AV_PIX_FMT_NV12,      FOURCC_NV12},
        {OMX_QCOM_COLOR_FormatYVU420SemiPlanar,                     AV_PIX_FMT_NV12,      FOURCC_NV12},
        {OMX_QCOM_COLOR_FormatYVU420PackedSemiPlanar32m4ka,         AV_PIX_FMT_NV12,      FOURCC_NV12},
        {OMX_QCOM_COLOR_FormatYUV420PackedSemiPlanar16m2ka,         AV_PIX_FMT_NV12,      FOURCC_NV12},
        {OMX_QCOM_COLOR_FormatYUV420PackedSemiPlanar64x32Tile2m8ka, AV_PIX_FMT_NV12,      FOURCC_NV12},
        {OMX_QCOM_COLOR_FORMATYUV420PackedSemiPlanar32m,            AV_PIX_FMT_NV12,      FOURCC_NV12},
        {OMX_QCOM_COLOR_FORMATYUV420PackedSemiPlanar32mMultiView,   AV_PIX_FMT_NV12,      FOURCC_NV12},
        {OMX_SEC_COLOR_FormatNV12TPhysicalAddress,                  AV_PIX_FMT_NV12,      FOURCC_NV12},
        {OMX_SEC_COLOR_FormatNV12LPhysicalAddress,                  AV_PIX_FMT_NV12,      FOURCC_NV12},
        {OMX_SEC_COLOR_FormatNV12LVirtualAddress,                   AV_PIX_FMT_NV12,      FOURCC_NV12},
        {OMX_SEC_COLOR_FormatNV12Tiled,                             AV_PIX_FMT_NV12,      FOURCC_NV12},
        {OMX_SEC_COLOR_FormatNV21LPhysicalAddress,                  AV_PIX_FMT_NV21,      FOURCC_NV12},
        {OMX_SEC_COLOR_FormatNV21Linear,                            AV_PIX_FMT_NV21,      FOURCC_NV12},
//        {OMX_TI_COLOR_FormatRawBayer10bitStereo,                    AV_PIX_FMT_NV12},
        {OMX_TI_COLOR_FormatYUV420PackedSemiPlanar,                 AV_PIX_FMT_NV12,      FOURCC_NV12},
        {0}
};
#ifdef __cplusplus
};
#endif

#endif //JS_MEDIACODEC_DEF
