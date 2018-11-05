#ifndef JS_NDK_MEDIACODEC_PROXY
#define JS_NDK_MEDIACODEC_PROXY

#include "js_constant.h"
#include <sys/types.h>
#include <android/native_window.h>

extern int __JS_NDK_MEDIACODEC_LINKED__;
//#define __JS_NATIVE_MEDIACODEC_API_LEVEL__ 16
#define __JS_NATIVE_MEDIACODEC_API_LEVEL__ 100


#define AMEDIACODEC_FLAG_CODEC_CONFIG  2

JS_RET link_ndk_mediacodec();

void close_ndk_mediacodec();

struct AMediaCodec;
typedef struct AMediaCodec AMediaCodec;

typedef struct JSMediaCodecBufferInfo AMediaCodecBufferInfo;
typedef struct AMediaCodecCryptoInfo AMediaCodecCryptoInfo;

struct AMediaFormat;
typedef struct AMediaFormat AMediaFormat;

struct AMediaCrypto;
typedef struct AMediaCrypto AMediaCrypto;

typedef enum {
    AMEDIA_OK = 0,

    AMEDIA_ERROR_BASE = -10000,
    AMEDIA_ERROR_UNKNOWN = AMEDIA_ERROR_BASE,
    AMEDIA_ERROR_MALFORMED = AMEDIA_ERROR_BASE - 1,
    AMEDIA_ERROR_UNSUPPORTED = AMEDIA_ERROR_BASE - 2,
    AMEDIA_ERROR_INVALID_OBJECT = AMEDIA_ERROR_BASE - 3,
    AMEDIA_ERROR_INVALID_PARAMETER = AMEDIA_ERROR_BASE - 4,

    AMEDIA_DRM_ERROR_BASE = -20000,
    AMEDIA_DRM_NOT_PROVISIONED = AMEDIA_DRM_ERROR_BASE - 1,
    AMEDIA_DRM_RESOURCE_BUSY = AMEDIA_DRM_ERROR_BASE - 2,
    AMEDIA_DRM_DEVICE_REVOKED = AMEDIA_DRM_ERROR_BASE - 3,
    AMEDIA_DRM_SHORT_BUFFER = AMEDIA_DRM_ERROR_BASE - 4,
    AMEDIA_DRM_SESSION_NOT_OPENED = AMEDIA_DRM_ERROR_BASE - 5,
    AMEDIA_DRM_TAMPER_DETECTED = AMEDIA_DRM_ERROR_BASE - 6,
    AMEDIA_DRM_VERIFY_FAILED = AMEDIA_DRM_ERROR_BASE - 7,
    AMEDIA_DRM_NEED_KEY = AMEDIA_DRM_ERROR_BASE - 8,
    AMEDIA_DRM_LICENSE_EXPIRED = AMEDIA_DRM_ERROR_BASE - 9,

} media_status_t;

AMediaCodec *AMediaCodec_createCodecByName(const char *name);

AMediaCodec *AMediaCodec_createDecoderByType(const char *mime_type);

AMediaCodec *AMediaCodec_createEncoderByType(const char *mime_type);

media_status_t AMediaCodec_delete(AMediaCodec *codec);

media_status_t AMediaCodec_configure(
        AMediaCodec *codec,
        const AMediaFormat *format,
        ANativeWindow *surface,
        AMediaCrypto *crypto,
        uint32_t flags);


media_status_t AMediaCodec_start(AMediaCodec *codec);

media_status_t AMediaCodec_stop(AMediaCodec *codec);

media_status_t AMediaCodec_flush(AMediaCodec *codec);

uint8_t *AMediaCodec_getInputBuffer(AMediaCodec *codec, size_t idx, size_t *out_size);

uint8_t *AMediaCodec_getOutputBuffer(AMediaCodec *codec, size_t idx, size_t *out_size);

ssize_t AMediaCodec_dequeueInputBuffer(AMediaCodec *codec, int64_t timeoutUs);

media_status_t AMediaCodec_queueInputBuffer(AMediaCodec *codec,
                                            size_t idx, off_t offset, size_t size, uint64_t time,
                                            uint32_t flags);

ssize_t
AMediaCodec_dequeueOutputBuffer(AMediaCodec *codec, AMediaCodecBufferInfo *info, int64_t timeoutUs);

AMediaFormat *AMediaCodec_getOutputFormat(AMediaCodec *codec);

media_status_t AMediaCodec_releaseOutputBuffer(AMediaCodec *codec, size_t idx, int render);

media_status_t
AMediaCodec_releaseOutputBufferAtTime(AMediaCodec *codec, size_t idx, int64_t timestampNs);

/////////////////AmediaFormat///////////////////////
AMediaFormat *AMediaFormat_new();

media_status_t AMediaFormat_delete(AMediaFormat *format);

const char *AMediaFormat_toString(AMediaFormat *format);

void AMediaFormat_setInt32(AMediaFormat *format, const char *name, int32_t value);

int AMediaFormat_getInt32(AMediaFormat *format, const char *name, int32_t *out);

void AMediaFormat_setBuffer(AMediaFormat *format, const char *name, void *data, size_t size);

int AMediaFormat_getBuffer(AMediaFormat *format, const char *name, void **data, size_t *size);

void AMediaFormat_setString(AMediaFormat *format, const char *name, const char *value);

int AMediaFormat_getString(AMediaFormat *format, const char *name, const char **out);

void AMediaFormat_setInt64(AMediaFormat *format, const char *name, int64_t value);

int AMediaFormat_getInt64(AMediaFormat *format, const char *name, int64_t *out);

void AMediaFormat_setFloat(AMediaFormat *format, const char *name, float value);

int AMediaFormat_getFloat(AMediaFormat *format, const char *name, float *out);

#endif /* JS_NDK_MEDIACODEC_PROXY */
