#include "js_ndk_mediacodec_proxy.h"
#include "util/js_log.h"
#include <dlfcn.h>
#include <pthread.h>
#include <stdio.h>


int __JS_NDK_MEDIACODEC_LINKED__ = 0;

typedef AMediaCodec *(*pf_AMediaCodec_createDecoderByType)(const char *mime_type);

typedef AMediaCodec *(*pf_AMediaCodec_createEncoderByType)(const char *mime_type);

typedef AMediaCodec *(*pf_AMediaCodec_createCodecByName)(const char *name);

typedef media_status_t(*pf_AMediaCodec_configure)(AMediaCodec *,
                                                  const AMediaFormat *format,
                                                  ANativeWindow *surface,
                                                  AMediaCrypto *crypto,
                                                  uint32_t flags);

typedef media_status_t(*pf_AMediaCodec_start)(AMediaCodec *);

typedef media_status_t(*pf_AMediaCodec_stop)(AMediaCodec *);

typedef media_status_t(*pf_AMediaCodec_flush)(AMediaCodec *);

typedef media_status_t(*pf_AMediaCodec_delete)(AMediaCodec *);

typedef AMediaFormat *(*pf_AMediaCodec_getOutputFormat)(AMediaCodec *);

typedef ssize_t(*pf_AMediaCodec_dequeueInputBuffer)(AMediaCodec *,
                                                    int64_t timeoutUs);

typedef uint8_t *(*pf_AMediaCodec_getInputBuffer)(AMediaCodec *,
                                                  size_t idx, size_t *out_size);

typedef media_status_t(*pf_AMediaCodec_queueInputBuffer)(AMediaCodec *,
                                                         size_t idx, off_t offset, size_t size,
                                                         uint64_t time, uint32_t flags);

typedef ssize_t(*pf_AMediaCodec_dequeueOutputBuffer)(AMediaCodec *,
                                                     AMediaCodecBufferInfo *info,
                                                     int64_t timeoutUs);

typedef uint8_t *(*pf_AMediaCodec_getOutputBuffer)(AMediaCodec *,
                                                   size_t idx, size_t *out_size);

typedef media_status_t(*pf_AMediaCodec_releaseOutputBuffer)(AMediaCodec *,
                                                            size_t idx, int render);

typedef media_status_t(*pf_AMediaCodec_releaseOutputBufferAtTime)(AMediaCodec *, size_t idx,
                                                                  int64_t timestampNs);

typedef AMediaFormat *(*pf_AMediaFormat_new)();

typedef media_status_t(*pf_AMediaFormat_delete)(AMediaFormat *);

typedef char *(*pf_AMediaFormat_toString)(AMediaFormat *);

typedef void(*pf_AMediaFormat_setString)(AMediaFormat *,
                                         const char *name, const char *value);

typedef int(*pf_AMediaFormat_getString)(AMediaFormat *,
                                        const char *name, const char **out);

typedef void(*pf_AMediaFormat_setBuffer)(AMediaFormat *, const char *name, void *data, size_t size);

typedef int(*pf_AMediaFormat_getBuffer)(AMediaFormat *, const char *name, void **data,
                                        size_t *size);

typedef void(*pf_AMediaFormat_setInt32)(AMediaFormat *,
                                        const char *name, int32_t value);

typedef int(*pf_AMediaFormat_getInt32)(AMediaFormat *,
                                       const char *name, int32_t *out);

typedef void(*pf_AMediaFormat_setInt64)(AMediaFormat *,
                                        const char *name, int64_t value);

typedef int(*pf_AMediaFormat_getInt64)(AMediaFormat *,
                                       const char *name, int64_t *out);

typedef void(*pf_AMediaFormat_setFloat)(AMediaFormat *,
                                        const char *name, float value);

typedef int(*pf_AMediaFormat_getFloat)(AMediaFormat *,
                                       const char *name, float *out);


struct JSMediaCodecProxy {
    struct {
        pf_AMediaCodec_createDecoderByType _createDecoderByType;
        pf_AMediaCodec_createEncoderByType _createEncoderByType;
        pf_AMediaCodec_createCodecByName _createCodecByName;
        pf_AMediaCodec_configure _configure;
        pf_AMediaCodec_start _start;
        pf_AMediaCodec_stop _stop;
        pf_AMediaCodec_flush _flush;
        pf_AMediaCodec_delete _delete;
        pf_AMediaCodec_getOutputFormat _getOutputFormat;
        pf_AMediaCodec_dequeueInputBuffer _dequeueInputBuffer;
        pf_AMediaCodec_getInputBuffer _getInputBuffer;
        pf_AMediaCodec_queueInputBuffer _queueInputBuffer;
        pf_AMediaCodec_dequeueOutputBuffer _dequeueOutputBuffer;
        pf_AMediaCodec_getOutputBuffer _getOutputBuffer;
        pf_AMediaCodec_releaseOutputBuffer _releaseOutputBuffer;
        pf_AMediaCodec_releaseOutputBufferAtTime _releaseOutputBufferAtTime;
    } AMediaCodec;
    struct {
        pf_AMediaFormat_new _new;
        pf_AMediaFormat_delete _delete;
        pf_AMediaFormat_toString _toString;
        pf_AMediaFormat_setString _setString;
        pf_AMediaFormat_getString _getString;
        pf_AMediaFormat_setBuffer _setBuffer;
        pf_AMediaFormat_getBuffer _getBuffer;
        pf_AMediaFormat_setInt32 _setInt32;
        pf_AMediaFormat_getInt32 _getInt32;
        pf_AMediaFormat_setInt64 _setInt64;
        pf_AMediaFormat_getInt64 _getInt64;
        pf_AMediaFormat_setFloat _setFloat;
        pf_AMediaFormat_getFloat _getFloat;
    } AMediaFormat;
};


static struct JSMediaCodecProxy proxy;

struct members {
    const char *name;
    int offset;
};
static struct members members[] =
        {
#define OFF(x) offsetof(struct JSMediaCodecProxy, AMediaCodec.x)
                {"AMediaCodec_createDecoderByType", OFF(_createDecoderByType)},
                {"AMediaCodec_createEncoderByType", OFF(_createEncoderByType)},
                {"AMediaCodec_createCodecByName", OFF(_createCodecByName)},
                {"AMediaCodec_configure", OFF(_configure)},
                {"AMediaCodec_start", OFF(_start)},
                {"AMediaCodec_stop", OFF(_stop)},
                {"AMediaCodec_flush", OFF(_flush)},
                {"AMediaCodec_delete", OFF(_delete)},
                {"AMediaCodec_getOutputFormat", OFF(_getOutputFormat)},
                {"AMediaCodec_dequeueInputBuffer", OFF(_dequeueInputBuffer)},
                {"AMediaCodec_getInputBuffer", OFF(_getInputBuffer)},
                {"AMediaCodec_queueInputBuffer", OFF(_queueInputBuffer)},
                {"AMediaCodec_dequeueOutputBuffer", OFF(_dequeueOutputBuffer)},
                {"AMediaCodec_getOutputBuffer", OFF(_getOutputBuffer)},
                {"AMediaCodec_releaseOutputBuffer", OFF(_releaseOutputBuffer)},
                {"AMediaCodec_releaseOutputBufferAtTime", OFF(_releaseOutputBufferAtTime)},
#undef OFF
#define OFF(x) offsetof(struct JSMediaCodecProxy, AMediaFormat.x)
                {"AMediaFormat_new", OFF(_new)},
                {"AMediaFormat_delete", OFF(_delete)},
                {"AMediaFormat_toString", OFF(_toString)},
                {"AMediaFormat_getBuffer", OFF(_getBuffer)},
                {"AMediaFormat_setBuffer", OFF(_setBuffer)},
                {"AMediaFormat_setString", OFF(_setString)},
                {"AMediaFormat_getString", OFF(_getString)},
                {"AMediaFormat_setInt32", OFF(_setInt32)},
                {"AMediaFormat_getInt32", OFF(_getInt32)},
                {"AMediaFormat_setInt64", OFF(_setInt64)},
                {"AMediaFormat_getInt64", OFF(_getInt64)},
                {"AMediaFormat_setFloat", OFF(_setFloat)},
                {"AMediaFormat_getFloat", OFF(_getFloat)},
#undef OFF
                {NULL, 0}
        };
#undef OFF

static void *ndk_handle = NULL;

JS_RET link_ndk_mediacodec() {

    JS_RET ret = JS_ERR;

    ndk_handle = dlopen("libmediandk.so", RTLD_NOW);

    if (!ndk_handle) {
        LOGE("%s failed to dlopen libmediandk.so", __func__);
        goto fail;
    }

    for (int i = 0; members[i].name; i++) {

        void *sym = dlsym(ndk_handle, members[i].name);
        if (!sym) {
            LOGE("%s failed to get %s func in libmediandk.so", __func__, members[i].name);
            goto fail;
        }
        *(void **) ((uint8_t *) &proxy + members[i].offset) = sym;
    }
    LOGD("%s dlopen libmediandk.so succeed", __func__);
    ret = JS_OK;
    return ret;

    fail:
    if (ndk_handle) {
        dlclose(ndk_handle);
    }

    return ret;
}

void close_ndk_mediacodec() {
    if (ndk_handle) {
        dlclose(ndk_handle);
    }
}

////////////////////AMediaCodec///////////////////

AMediaCodec *AMediaCodec_createCodecByName(const char *name) {
    return proxy.AMediaCodec._createCodecByName(name);
}

AMediaCodec *AMediaCodec_createDecoderByType(const char *mime_type) {
    return proxy.AMediaCodec._createDecoderByType(mime_type);
}

AMediaCodec *AMediaCodec_createEncoderByType(const char *mime_type) {
    return proxy.AMediaCodec._createEncoderByType(mime_type);
}

media_status_t AMediaCodec_delete(AMediaCodec *codec) {
    return proxy.AMediaCodec._delete(codec);
}

media_status_t AMediaCodec_configure(
        AMediaCodec *codec,
        const AMediaFormat *format,
        ANativeWindow *surface,
        AMediaCrypto *crypto,
        uint32_t flags) {
    return proxy.AMediaCodec._configure(codec, format, surface, crypto, flags);
}

media_status_t AMediaCodec_start(AMediaCodec *codec) {
    return proxy.AMediaCodec._start(codec);
}

media_status_t AMediaCodec_stop(AMediaCodec *codec) {
    return proxy.AMediaCodec._stop(codec);
}

media_status_t AMediaCodec_flush(AMediaCodec *codec) {
    return proxy.AMediaCodec._flush(codec);
}

uint8_t *AMediaCodec_getInputBuffer(AMediaCodec *codec, size_t idx, size_t *out_size) {
    return proxy.AMediaCodec._getInputBuffer(codec, idx, out_size);
}

uint8_t *AMediaCodec_getOutputBuffer(AMediaCodec *codec, size_t idx, size_t *out_size) {
    return proxy.AMediaCodec._getOutputBuffer(codec, idx, out_size);
}

ssize_t AMediaCodec_dequeueInputBuffer(AMediaCodec *codec, int64_t timeoutUs) {
    return proxy.AMediaCodec._dequeueInputBuffer(codec, timeoutUs);
}

media_status_t AMediaCodec_queueInputBuffer(AMediaCodec *codec,
                                            size_t idx, off_t offset, size_t size, uint64_t time,
                                            uint32_t flags) {
    return proxy.AMediaCodec._queueInputBuffer(codec, idx, offset, size, time, flags);
}

ssize_t AMediaCodec_dequeueOutputBuffer(AMediaCodec *codec, AMediaCodecBufferInfo *info,
                                        int64_t timeoutUs) {
    return proxy.AMediaCodec._dequeueOutputBuffer(codec, info, timeoutUs);
}

AMediaFormat *AMediaCodec_getOutputFormat(AMediaCodec *codec) {
    return proxy.AMediaCodec._getOutputFormat(codec);
}

media_status_t AMediaCodec_releaseOutputBuffer(AMediaCodec *codec, size_t idx, int render) {
    return proxy.AMediaCodec._releaseOutputBuffer(codec, idx, render);
}

media_status_t
AMediaCodec_releaseOutputBufferAtTime(AMediaCodec *codec, size_t idx, int64_t timestampNs) {
    return proxy.AMediaCodec._releaseOutputBufferAtTime(codec, idx, timestampNs);
}

////////////////////AmediaFormat/////////////////////
AMediaFormat *AMediaFormat_new() {
    return proxy.AMediaFormat._new();
}

media_status_t AMediaFormat_delete(AMediaFormat *format) {
    return proxy.AMediaFormat._delete(format);
}

const char *AMediaFormat_toString(AMediaFormat *format) {
    return proxy.AMediaFormat._toString(format);
}

void AMediaFormat_setInt32(AMediaFormat *format, const char *name, int32_t value) {
    return proxy.AMediaFormat._setInt32(format, name, value);
}

int AMediaFormat_getInt32(AMediaFormat *format, const char *name, int32_t *out) {
    return proxy.AMediaFormat._getInt32(format, name, out);
}

void AMediaFormat_setBuffer(AMediaFormat *format, const char *name, void *data, size_t size) {
    return proxy.AMediaFormat._setBuffer(format, name, data, size);
}

int AMediaFormat_getBuffer(AMediaFormat *format, const char *name, void **data, size_t *size) {
    return proxy.AMediaFormat._getBuffer(format, name, data, size);
}

void AMediaFormat_setString(AMediaFormat *format, const char *name, const char *value) {
    return proxy.AMediaFormat._setString(format, name, value);
}

int AMediaFormat_getString(AMediaFormat *format, const char *name, const char **out) {
    return proxy.AMediaFormat._getString(format, name, out);
}

void AMediaFormat_setInt64(AMediaFormat *format, const char *name, int64_t value) {
    return proxy.AMediaFormat._setInt64(format, name, value);
}

int AMediaFormat_getInt64(AMediaFormat *format, const char *name, int64_t *out) {
    return proxy.AMediaFormat._getInt64(format, name, out);
}

void AMediaFormat_setFloat(AMediaFormat *format, const char *name, float value) {
    return proxy.AMediaFormat._setFloat(format, name, value);
}

int AMediaFormat_getFloat(AMediaFormat *format, const char *name, float *out) {
    return proxy.AMediaFormat._getFloat(format, name, out);
}
