#ifndef JS_JAVA_MEDIACODEC_WRAPPER_H
#define JS_JAVA_MEDIACODEC_WRAPPER_H

#include "libavcodec/avcodec.h"

/**
 * The following API around MediaCodec and MediaFormat is based on the
 * NDK one provided by Google since Android 5.0.
 *
 * Differences from the NDK API:
 *
 * Buffers returned by js_MediaFormat_toString and js_MediaFormat_getString
 * are newly allocated buffer and must be freed by the user after use.
 *
 * The MediaCrypto API is not implemented.
 *
 * js_MediaCodec_infoTryAgainLater, js_MediaCodec_infoOutputBuffersChanged,
 * js_MediaCodec_infoOutputFormatChanged, js_MediaCodec_cleanOutputBuffers
 * js_MediaCodec_getName and js_MediaCodec_getBufferFlagEndOfStream are not
 * part of the original NDK API and are convenience functions to hide JNI
 * implementation.
 *
 * The API around MediaCodecList is not part of the NDK (and is lacking as
 * we still need to retrieve the codec name to work around faulty decoders
 * and encoders).
 *
 * For documentation, please refers to NdkMediaCodec.h NdkMediaFormat.h and
 * http://developer.android.com/reference/android/media/MediaCodec.html.
 *
 */



int js_parseAVCodecContext(char *mime_type, int *width, int *height, int *profile, int *level,
                           AVCodecContext *avctx);

char *
js_MediaCodecList_getCodecNameByType(const char *mime, int profile, int encoder, void *log_ctx);

struct JSMediaFormat;
typedef struct JSMediaFormat JSMediaFormat;

void *js_MediaFormat_new(void);

int js_MediaFormat_delete(void *format);

char *js_MediaFormat_toString(void *format);

int js_MediaFormat_getInt32(void *format, const char *name, int32_t *out);

int js_MediaFormat_getInt64(void *format, const char *name, int64_t *out);

int js_MediaFormat_getFloat(void *format, const char *name, float *out);

int js_MediaFormat_getBuffer(void *format, const char *name, void **data, size_t *size);

int js_MediaFormat_getString(void *format, const char *name, const char **out);

void js_MediaFormat_setInt32(void *format, const char *name, int32_t value);

void js_MediaFormat_setInt64(void *format, const char *name, int64_t value);

void js_MediaFormat_setFloat(void *format, const char *name, float value);

void js_MediaFormat_setString(void *format, const char *name, const char *value);

void js_MediaFormat_setBuffer(void *format, const char *name, void *data, size_t size);

struct JSMediaCodec;
typedef struct JSMediaCodec JSMediaCodec;
typedef struct JSMediaCodecCryptoInfo JSMediaCodecCryptoInfo;

struct JSMediaCodecBufferInfo {
    int32_t offset;
    int32_t size;
    int64_t presentationTimeUs;
    uint32_t flags;
};
typedef struct JSMediaCodecBufferInfo JSMediaCodecBufferInfo;

char *js_MediaCodec_getName(JSMediaCodec *codec);

void *js_MediaCodec_createCodecByName(const char *name);

void *js_MediaCodec_createDecoderByType(const char *mime_type);

void *js_MediaCodec_createEncoderByType(const char *mime_type);

int js_MediaCodec_configure(void *codec, const void *format, void *surface,
                            void *crypto, uint32_t flags);

int js_MediaCodec_start(void *codec);

int js_MediaCodec_stop(void *codec);

int js_MediaCodec_flush(void *codec);

int js_MediaCodec_delete(void *codec);

uint8_t *js_MediaCodec_getInputBuffer(void *codec, size_t idx, size_t *out_size);

uint8_t *js_MediaCodec_getOutputBuffer(void *codec, size_t idx, size_t *out_size);

ssize_t js_MediaCodec_dequeueInputBuffer(void *codec, int64_t timeoutUs);

int js_MediaCodec_queueInputBuffer(void *codec, size_t idx, off_t offset, size_t size,
                                   uint64_t time, uint32_t flags);

ssize_t js_MediaCodec_dequeueOutputBuffer(void *codec, JSMediaCodecBufferInfo *info,
                                          int64_t timeoutUs);

void *js_MediaCodec_getOutputFormat(void *codec);

int js_MediaCodec_releaseOutputBuffer(void *codec, size_t idx, int render);

int js_MediaCodec_releaseOutputBufferAtTime(void *codec, size_t idx, int64_t timestampNs);

int js_MediaCodec_getInfoTryAgainLater(JSMediaCodec *codec);

int js_MediaCodec_getInfoOutputBuffersChanged(JSMediaCodec *codec);

int js_MediaCodec_getInfoOutputFormatChanged(JSMediaCodec *codec);

int js_MediaCodec_getBufferFlagCodecConfig(JSMediaCodec *codec);

int js_MediaCodec_getBufferFlagEndOfStream(JSMediaCodec *codec);

int js_MediaCodec_getBufferFlagKeyFrame(JSMediaCodec *codec);

int js_MediaCodec_getConfigureFlagEncode(JSMediaCodec *codec);

int js_MediaCodec_cleanOutputBuffers(JSMediaCodec *codec);


#endif /* JS_JAVA_MEDIACODEC_WRAPPER_H */
