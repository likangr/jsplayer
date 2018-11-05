#include "js_java_mediacodec_wrapper.h"
#include "js_ndk_mediacodec_proxy.h" //todo 函数指针优化，减少if else
#include "js_mediacodec_def.h"
#include "util/js_jni.h"
#include "util/js_log.h"
#include "libavutil/avassert.h"
#include "libavutil/avstring.h"


struct JNIMediaCodecListFields {

    jclass mediacodec_list_class;
    jmethodID init_id;
    jmethodID find_decoder_for_format_id;

    jmethodID get_codec_count_id;
    jmethodID get_codec_info_at_id;

    jclass mediacodec_info_class;
    jmethodID get_name_id;
    jmethodID get_codec_capabilities_id;
    jmethodID get_supported_types_id;
    jmethodID is_encoder_id;

    jclass codec_capabilities_class;
    jfieldID color_formats_id;
    jfieldID profile_levels_id;

    jclass codec_profile_level_class;
    jfieldID profile_id;
    jfieldID level_id;

    jfieldID avc_profile_baseline_id;
    jfieldID avc_profile_main_id;
    jfieldID avc_profile_extended_id;
    jfieldID avc_profile_high_id;
    jfieldID avc_profile_high10_id;
    jfieldID avc_profile_high422_id;
    jfieldID avc_profile_high444_id;

    jfieldID hevc_profile_main_id;
    jfieldID hevc_profile_main10_id;
    jfieldID hevc_profile_main10_hdr10_id;

};

static const struct JSJniField jni_mediacodeclist_mapping[] = {
        {"android/media/MediaCodecList",                   NULL, NULL,                                                                                       JS_JNI_CLASS,         offsetof(
                                                                                                                                                                                           struct JNIMediaCodecListFields,
                                                                                                                                                                                           mediacodec_list_class),        1},
        {"android/media/MediaCodecList",                   "<init>",                 "(I)V",                                                                 JS_JNI_METHOD,        offsetof(
                                                                                                                                                                                           struct JNIMediaCodecListFields,
                                                                                                                                                                                           init_id),                      0},
        {"android/media/MediaCodecList",                   "findDecoderForFormat",   "(Landroid/media/MediaFormat;)Ljava/lang/String;",                      JS_JNI_METHOD,        offsetof(
                                                                                                                                                                                           struct JNIMediaCodecListFields,
                                                                                                                                                                                           find_decoder_for_format_id),   0},

        {"android/media/MediaCodecList",                   "getCodecCount",          "()I",                                                                  JS_JNI_STATIC_METHOD, offsetof(
                                                                                                                                                                                           struct JNIMediaCodecListFields,
                                                                                                                                                                                           get_codec_count_id),           1},
        {"android/media/MediaCodecList",                   "getCodecInfoAt",         "(I)Landroid/media/MediaCodecInfo;",                                    JS_JNI_STATIC_METHOD, offsetof(
                                                                                                                                                                                           struct JNIMediaCodecListFields,
                                                                                                                                                                                           get_codec_info_at_id),         1},

        {"android/media/MediaCodecInfo",                   NULL, NULL,                                                                                       JS_JNI_CLASS,         offsetof(
                                                                                                                                                                                           struct JNIMediaCodecListFields,
                                                                                                                                                                                           mediacodec_info_class),        1},
        {"android/media/MediaCodecInfo",                   "getName",                "()Ljava/lang/String;",                                                 JS_JNI_METHOD,        offsetof(
                                                                                                                                                                                           struct JNIMediaCodecListFields,
                                                                                                                                                                                           get_name_id),                  1},
        {"android/media/MediaCodecInfo",                   "getCapabilitiesForType", "(Ljava/lang/String;)Landroid/media/MediaCodecInfo$CodecCapabilities;", JS_JNI_METHOD,        offsetof(
                                                                                                                                                                                           struct JNIMediaCodecListFields,
                                                                                                                                                                                           get_codec_capabilities_id),    1},
        {"android/media/MediaCodecInfo",                   "getSupportedTypes",      "()[Ljava/lang/String;",                                                JS_JNI_METHOD,        offsetof(
                                                                                                                                                                                           struct JNIMediaCodecListFields,
                                                                                                                                                                                           get_supported_types_id),       1},
        {"android/media/MediaCodecInfo",                   "isEncoder",              "()Z",                                                                  JS_JNI_METHOD,        offsetof(
                                                                                                                                                                                           struct JNIMediaCodecListFields,
                                                                                                                                                                                           is_encoder_id),                1},

        {"android/media/MediaCodecInfo$CodecCapabilities", NULL, NULL,                                                                                       JS_JNI_CLASS,         offsetof(
                                                                                                                                                                                           struct JNIMediaCodecListFields,
                                                                                                                                                                                           codec_capabilities_class),     1},
        {"android/media/MediaCodecInfo$CodecCapabilities", "colorFormats",           "[I",                                                                   JS_JNI_FIELD,         offsetof(
                                                                                                                                                                                           struct JNIMediaCodecListFields,
                                                                                                                                                                                           color_formats_id),             1},
        {"android/media/MediaCodecInfo$CodecCapabilities", "profileLevels",          "[Landroid/media/MediaCodecInfo$CodecProfileLevel;",                    JS_JNI_FIELD,         offsetof(
                                                                                                                                                                                           struct JNIMediaCodecListFields,
                                                                                                                                                                                           profile_levels_id),            1},

        {"android/media/MediaCodecInfo$CodecProfileLevel", NULL, NULL,                                                                                       JS_JNI_CLASS,         offsetof(
                                                                                                                                                                                           struct JNIMediaCodecListFields,
                                                                                                                                                                                           codec_profile_level_class),    1},
        {"android/media/MediaCodecInfo$CodecProfileLevel", "profile",                "I",                                                                    JS_JNI_FIELD,         offsetof(
                                                                                                                                                                                           struct JNIMediaCodecListFields,
                                                                                                                                                                                           profile_id),                   1},
        {"android/media/MediaCodecInfo$CodecProfileLevel", "level",                  "I",                                                                    JS_JNI_FIELD,         offsetof(
                                                                                                                                                                                           struct JNIMediaCodecListFields,
                                                                                                                                                                                           level_id),                     1},

        {"android/media/MediaCodecInfo$CodecProfileLevel", "AVCProfileBaseline",     "I",                                                                    JS_JNI_STATIC_FIELD,  offsetof(
                                                                                                                                                                                           struct JNIMediaCodecListFields,
                                                                                                                                                                                           avc_profile_baseline_id),      1},
        {"android/media/MediaCodecInfo$CodecProfileLevel", "AVCProfileMain",         "I",                                                                    JS_JNI_STATIC_FIELD,  offsetof(
                                                                                                                                                                                           struct JNIMediaCodecListFields,
                                                                                                                                                                                           avc_profile_main_id),          1},
        {"android/media/MediaCodecInfo$CodecProfileLevel", "AVCProfileExtended",     "I",                                                                    JS_JNI_STATIC_FIELD,  offsetof(
                                                                                                                                                                                           struct JNIMediaCodecListFields,
                                                                                                                                                                                           avc_profile_extended_id),      1},
        {"android/media/MediaCodecInfo$CodecProfileLevel", "AVCProfileHigh",         "I",                                                                    JS_JNI_STATIC_FIELD,  offsetof(
                                                                                                                                                                                           struct JNIMediaCodecListFields,
                                                                                                                                                                                           avc_profile_high_id),          1},
        {"android/media/MediaCodecInfo$CodecProfileLevel", "AVCProfileHigh10",       "I",                                                                    JS_JNI_STATIC_FIELD,  offsetof(
                                                                                                                                                                                           struct JNIMediaCodecListFields,
                                                                                                                                                                                           avc_profile_high10_id),        1},
        {"android/media/MediaCodecInfo$CodecProfileLevel", "AVCProfileHigh422",      "I",                                                                    JS_JNI_STATIC_FIELD,  offsetof(
                                                                                                                                                                                           struct JNIMediaCodecListFields,
                                                                                                                                                                                           avc_profile_high422_id),       1},
        {"android/media/MediaCodecInfo$CodecProfileLevel", "AVCProfileHigh444",      "I",                                                                    JS_JNI_STATIC_FIELD,  offsetof(
                                                                                                                                                                                           struct JNIMediaCodecListFields,
                                                                                                                                                                                           avc_profile_high444_id),       1},

        {"android/media/MediaCodecInfo$CodecProfileLevel", "HEVCProfileMain",        "I",                                                                    JS_JNI_STATIC_FIELD,  offsetof(
                                                                                                                                                                                           struct JNIMediaCodecListFields,
                                                                                                                                                                                           hevc_profile_main_id),         0},
        {"android/media/MediaCodecInfo$CodecProfileLevel", "HEVCProfileMain10",      "I",                                                                    JS_JNI_STATIC_FIELD,  offsetof(
                                                                                                                                                                                           struct JNIMediaCodecListFields,
                                                                                                                                                                                           hevc_profile_main10_id),       0},
        {"android/media/MediaCodecInfo$CodecProfileLevel", "HEVCProfileMain10HDR10", "I",                                                                    JS_JNI_STATIC_FIELD,  offsetof(
                                                                                                                                                                                           struct JNIMediaCodecListFields,
                                                                                                                                                                                           hevc_profile_main10_hdr10_id), 0},

        {NULL}
};

struct JNIMediaFormatFields {

    jclass mediaformat_class;

    jmethodID init_id;

    jmethodID get_integer_id;
    jmethodID get_long_id;
    jmethodID get_float_id;
    jmethodID get_bytebuffer_id;
    jmethodID get_string_id;

    jmethodID set_integer_id;
    jmethodID set_long_id;
    jmethodID set_float_id;
    jmethodID set_bytebuffer_id;
    jmethodID set_string_id;

    jmethodID to_string_id;

};

static const struct JSJniField jni_mediaformat_mapping[] = {
        {"android/media/MediaFormat", NULL, NULL,                                                    JS_JNI_CLASS,  offsetof(
                                                                                                                            struct JNIMediaFormatFields,
                                                                                                                            mediaformat_class), 1},

        {"android/media/MediaFormat", "<init>",        "()V",                                        JS_JNI_METHOD, offsetof(
                                                                                                                            struct JNIMediaFormatFields,
                                                                                                                            init_id),           1},

        {"android/media/MediaFormat", "getInteger",    "(Ljava/lang/String;)I",                      JS_JNI_METHOD, offsetof(
                                                                                                                            struct JNIMediaFormatFields,
                                                                                                                            get_integer_id),    1},
        {"android/media/MediaFormat", "getLong",       "(Ljava/lang/String;)J",                      JS_JNI_METHOD, offsetof(
                                                                                                                            struct JNIMediaFormatFields,
                                                                                                                            get_long_id),       1},
        {"android/media/MediaFormat", "getFloat",      "(Ljava/lang/String;)F",                      JS_JNI_METHOD, offsetof(
                                                                                                                            struct JNIMediaFormatFields,
                                                                                                                            get_float_id),      1},
        {"android/media/MediaFormat", "getByteBuffer", "(Ljava/lang/String;)Ljava/nio/ByteBuffer;",  JS_JNI_METHOD, offsetof(
                                                                                                                            struct JNIMediaFormatFields,
                                                                                                                            get_bytebuffer_id), 1},
        {"android/media/MediaFormat", "getString",     "(Ljava/lang/String;)Ljava/lang/String;",     JS_JNI_METHOD, offsetof(
                                                                                                                            struct JNIMediaFormatFields,
                                                                                                                            get_string_id),     1},

        {"android/media/MediaFormat", "setInteger",    "(Ljava/lang/String;I)V",                     JS_JNI_METHOD, offsetof(
                                                                                                                            struct JNIMediaFormatFields,
                                                                                                                            set_integer_id),    1},
        {"android/media/MediaFormat", "setLong",       "(Ljava/lang/String;J)V",                     JS_JNI_METHOD, offsetof(
                                                                                                                            struct JNIMediaFormatFields,
                                                                                                                            set_long_id),       1},
        {"android/media/MediaFormat", "setFloat",      "(Ljava/lang/String;F)V",                     JS_JNI_METHOD, offsetof(
                                                                                                                            struct JNIMediaFormatFields,
                                                                                                                            set_float_id),      1},
        {"android/media/MediaFormat", "setByteBuffer", "(Ljava/lang/String;Ljava/nio/ByteBuffer;)V", JS_JNI_METHOD, offsetof(
                                                                                                                            struct JNIMediaFormatFields,
                                                                                                                            set_bytebuffer_id), 1},
        {"android/media/MediaFormat", "setString",     "(Ljava/lang/String;Ljava/lang/String;)V",    JS_JNI_METHOD, offsetof(
                                                                                                                            struct JNIMediaFormatFields,
                                                                                                                            set_string_id),     1},

        {"android/media/MediaFormat", "toString",      "()Ljava/lang/String;",                       JS_JNI_METHOD, offsetof(
                                                                                                                            struct JNIMediaFormatFields,
                                                                                                                            to_string_id),      1},

        {NULL}
};

static const AVClass mediaformat_class = {
        .class_name = "mediaformat",
        .item_name  = av_default_item_name,
        .version    = LIBAVCODEC_VERSION_INT,
};

struct JSMediaFormat {

    const AVClass *class;
    struct JNIMediaFormatFields jfields;
    jobject object;
};

struct JNIMediaCodecFields {

    jclass mediacodec_class;

    jfieldID info_try_again_later_id;
    jfieldID info_output_buffers_changed_id;
    jfieldID info_output_format_changed_id;

    jfieldID buffer_flag_codec_config_id;
    jfieldID buffer_flag_end_of_stream_id;
    jfieldID buffer_flag_key_frame_id;

    jfieldID configure_flag_encode_id;

    jmethodID create_by_codec_name_id;
    jmethodID create_decoder_by_type_id;
    jmethodID create_encoder_by_type_id;

    jmethodID get_name_id;

    jmethodID configure_id;
    jmethodID start_id;
    jmethodID flush_id;
    jmethodID stop_id;
    jmethodID release_id;

    jmethodID get_output_format_id;

    jmethodID dequeue_input_buffer_id;
    jmethodID queue_input_buffer_id;
    jmethodID get_input_buffer_id;
    jmethodID get_input_buffers_id;

    jmethodID dequeue_output_buffer_id;
    jmethodID get_output_buffer_id;
    jmethodID get_output_buffers_id;
    jmethodID release_output_buffer_id;
    jmethodID release_output_buffer_at_time_id;

    jclass mediainfo_class;

    jmethodID init_id;

    jfieldID flags_id;
    jfieldID offset_id;
    jfieldID presentation_time_us_id;
    jfieldID size_id;

};

static const struct JSJniField jni_mediacodec_mapping[] = {
        {"android/media/MediaCodec",            NULL, NULL,                                                                                                        JS_JNI_CLASS,         offsetof(
                                                                                                                                                                                                 struct JNIMediaCodecFields,
                                                                                                                                                                                                 mediacodec_class),                 1},

        {"android/media/MediaCodec",            "INFO_TRY_AGAIN_LATER",        "I",                                                                                JS_JNI_STATIC_FIELD,  offsetof(
                                                                                                                                                                                                 struct JNIMediaCodecFields,
                                                                                                                                                                                                 info_try_again_later_id),          1},
        {"android/media/MediaCodec",            "INFO_OUTPUT_BUFFERS_CHANGED", "I",                                                                                JS_JNI_STATIC_FIELD,  offsetof(
                                                                                                                                                                                                 struct JNIMediaCodecFields,
                                                                                                                                                                                                 info_output_buffers_changed_id),   1},
        {"android/media/MediaCodec",            "INFO_OUTPUT_FORMAT_CHANGED",  "I",                                                                                JS_JNI_STATIC_FIELD,  offsetof(
                                                                                                                                                                                                 struct JNIMediaCodecFields,
                                                                                                                                                                                                 info_output_format_changed_id),    1},

        {"android/media/MediaCodec",            "BUFFER_FLAG_CODEC_CONFIG",    "I",                                                                                JS_JNI_STATIC_FIELD,  offsetof(
                                                                                                                                                                                                 struct JNIMediaCodecFields,
                                                                                                                                                                                                 buffer_flag_codec_config_id),      1},
        {"android/media/MediaCodec",            "BUFFER_FLAG_END_OF_STREAM",   "I",                                                                                JS_JNI_STATIC_FIELD,  offsetof(
                                                                                                                                                                                                 struct JNIMediaCodecFields,
                                                                                                                                                                                                 buffer_flag_end_of_stream_id),     1},
        {"android/media/MediaCodec",            "BUFFER_FLAG_KEY_FRAME",       "I",                                                                                JS_JNI_STATIC_FIELD,  offsetof(
                                                                                                                                                                                                 struct JNIMediaCodecFields,
                                                                                                                                                                                                 buffer_flag_key_frame_id),         0},

        {"android/media/MediaCodec",            "CONFIGURE_FLAG_ENCODE",       "I",                                                                                JS_JNI_STATIC_FIELD,  offsetof(
                                                                                                                                                                                                 struct JNIMediaCodecFields,
                                                                                                                                                                                                 configure_flag_encode_id),         1},

        {"android/media/MediaCodec",            "createByCodecName",           "(Ljava/lang/String;)Landroid/media/MediaCodec;",                                   JS_JNI_STATIC_METHOD, offsetof(
                                                                                                                                                                                                 struct JNIMediaCodecFields,
                                                                                                                                                                                                 create_by_codec_name_id),          1},
        {"android/media/MediaCodec",            "createDecoderByType",         "(Ljava/lang/String;)Landroid/media/MediaCodec;",                                   JS_JNI_STATIC_METHOD, offsetof(
                                                                                                                                                                                                 struct JNIMediaCodecFields,
                                                                                                                                                                                                 create_decoder_by_type_id),        1},
        {"android/media/MediaCodec",            "createEncoderByType",         "(Ljava/lang/String;)Landroid/media/MediaCodec;",                                   JS_JNI_STATIC_METHOD, offsetof(
                                                                                                                                                                                                 struct JNIMediaCodecFields,
                                                                                                                                                                                                 create_encoder_by_type_id),        1},

        {"android/media/MediaCodec",            "getName",                     "()Ljava/lang/String;",                                                             JS_JNI_METHOD,        offsetof(
                                                                                                                                                                                                 struct JNIMediaCodecFields,
                                                                                                                                                                                                 get_name_id),                      1},

        {"android/media/MediaCodec",            "configure",                   "(Landroid/media/MediaFormat;Landroid/view/Surface;Landroid/media/MediaCrypto;I)V", JS_JNI_METHOD,        offsetof(
                                                                                                                                                                                                 struct JNIMediaCodecFields,
                                                                                                                                                                                                 configure_id),                     1},
        {"android/media/MediaCodec",            "start",                       "()V",                                                                              JS_JNI_METHOD,        offsetof(
                                                                                                                                                                                                 struct JNIMediaCodecFields,
                                                                                                                                                                                                 start_id),                         1},
        {"android/media/MediaCodec",            "flush",                       "()V",                                                                              JS_JNI_METHOD,        offsetof(
                                                                                                                                                                                                 struct JNIMediaCodecFields,
                                                                                                                                                                                                 flush_id),                         1},
        {"android/media/MediaCodec",            "stop",                        "()V",                                                                              JS_JNI_METHOD,        offsetof(
                                                                                                                                                                                                 struct JNIMediaCodecFields,
                                                                                                                                                                                                 stop_id),                          1},
        {"android/media/MediaCodec",            "release",                     "()V",                                                                              JS_JNI_METHOD,        offsetof(
                                                                                                                                                                                                 struct JNIMediaCodecFields,
                                                                                                                                                                                                 release_id),                       1},

        {"android/media/MediaCodec",            "getOutputFormat",             "()Landroid/media/MediaFormat;",                                                    JS_JNI_METHOD,        offsetof(
                                                                                                                                                                                                 struct JNIMediaCodecFields,
                                                                                                                                                                                                 get_output_format_id),             1},

        {"android/media/MediaCodec",            "dequeueInputBuffer",          "(J)I",                                                                             JS_JNI_METHOD,        offsetof(
                                                                                                                                                                                                 struct JNIMediaCodecFields,
                                                                                                                                                                                                 dequeue_input_buffer_id),          1},
        {"android/media/MediaCodec",            "queueInputBuffer",            "(IIIJI)V",                                                                         JS_JNI_METHOD,        offsetof(
                                                                                                                                                                                                 struct JNIMediaCodecFields,
                                                                                                                                                                                                 queue_input_buffer_id),            1},
        {"android/media/MediaCodec",            "getInputBuffer",              "(I)Ljava/nio/ByteBuffer;",                                                         JS_JNI_METHOD,        offsetof(
                                                                                                                                                                                                 struct JNIMediaCodecFields,
                                                                                                                                                                                                 get_input_buffer_id),              0},
        {"android/media/MediaCodec",            "getInputBuffers",             "()[Ljava/nio/ByteBuffer;",                                                         JS_JNI_METHOD,        offsetof(
                                                                                                                                                                                                 struct JNIMediaCodecFields,
                                                                                                                                                                                                 get_input_buffers_id),             1},

        {"android/media/MediaCodec",            "dequeueOutputBuffer",         "(Landroid/media/MediaCodec$BufferInfo;J)I",                                        JS_JNI_METHOD,        offsetof(
                                                                                                                                                                                                 struct JNIMediaCodecFields,
                                                                                                                                                                                                 dequeue_output_buffer_id),         1},
        {"android/media/MediaCodec",            "getOutputBuffer",             "(I)Ljava/nio/ByteBuffer;",                                                         JS_JNI_METHOD,        offsetof(
                                                                                                                                                                                                 struct JNIMediaCodecFields,
                                                                                                                                                                                                 get_output_buffer_id),             0},
        {"android/media/MediaCodec",            "getOutputBuffers",            "()[Ljava/nio/ByteBuffer;",                                                         JS_JNI_METHOD,        offsetof(
                                                                                                                                                                                                 struct JNIMediaCodecFields,
                                                                                                                                                                                                 get_output_buffers_id),            1},
        {"android/media/MediaCodec",            "releaseOutputBuffer",         "(IZ)V",                                                                            JS_JNI_METHOD,        offsetof(
                                                                                                                                                                                                 struct JNIMediaCodecFields,
                                                                                                                                                                                                 release_output_buffer_id),         1},
        {"android/media/MediaCodec",            "releaseOutputBuffer",         "(IJ)V",                                                                            JS_JNI_METHOD,        offsetof(
                                                                                                                                                                                                 struct JNIMediaCodecFields,
                                                                                                                                                                                                 release_output_buffer_at_time_id), 0},

        {"android/media/MediaCodec$BufferInfo", NULL, NULL,                                                                                                        JS_JNI_CLASS,         offsetof(
                                                                                                                                                                                                 struct JNIMediaCodecFields,
                                                                                                                                                                                                 mediainfo_class),                  1},

        {"android/media/MediaCodec.BufferInfo", "<init>",                      "()V",                                                                              JS_JNI_METHOD,        offsetof(
                                                                                                                                                                                                 struct JNIMediaCodecFields,
                                                                                                                                                                                                 init_id),                          1},
        {"android/media/MediaCodec.BufferInfo", "flags",                       "I",                                                                                JS_JNI_FIELD,         offsetof(
                                                                                                                                                                                                 struct JNIMediaCodecFields,
                                                                                                                                                                                                 flags_id),                         1},
        {"android/media/MediaCodec.BufferInfo", "offset",                      "I",                                                                                JS_JNI_FIELD,         offsetof(
                                                                                                                                                                                                 struct JNIMediaCodecFields,
                                                                                                                                                                                                 offset_id),                        1},
        {"android/media/MediaCodec.BufferInfo", "presentationTimeUs",          "J",                                                                                JS_JNI_FIELD,         offsetof(
                                                                                                                                                                                                 struct JNIMediaCodecFields,
                                                                                                                                                                                                 presentation_time_us_id),          1},
        {"android/media/MediaCodec.BufferInfo", "size",                        "I",                                                                                JS_JNI_FIELD,         offsetof(
                                                                                                                                                                                                 struct JNIMediaCodecFields,
                                                                                                                                                                                                 size_id),                          1},

        {NULL}
};

static const AVClass mediacodec_class = {
        .class_name = "mediacodec",
        .item_name  = av_default_item_name,
        .version    = LIBAVCODEC_VERSION_INT,
};

struct JSMediaCodec {

    const AVClass *class;

    struct JNIMediaCodecFields jfields;

    jobject object;

    jobject input_buffers;
    jobject output_buffers;

    int JS_INFO_TRY_AGAIN_LATER;
    int JS_INFO_OUTPUT_BUFFERS_CHANGED;
    int JS_INFO_OUTPUT_FORMAT_CHANGED;

    int JS_BUFFER_FLAG_CODEC_CONFIG;
    int JS_BUFFER_FLAG_END_OF_STREAM;
    int JS_BUFFER_FLAG_KEY_FRAME;

    int JS_CONFIGURE_FLAG_ENCODE;

    int has_get_i_o_buffer;
};

#define JNI_GET_ENV_OR_RETURN(env, log_ctx, ret) do {              \
    (env) = js_jni_get_env(log_ctx);                               \
    if (!(env)) {                                                  \
        return ret;                                                \
    }                                                              \
} while (0)

#define JNI_GET_ENV_OR_RETURN_VOID(env, log_ctx) do {              \
    (env) = js_jni_get_env(log_ctx);                               \
    if (!(env)) {                                                  \
        return;                                                    \
    }                                                              \
} while (0)

int js_parseAVCodecContext(char *mime_type, int *width, int *height, int *profile, int *level,
                           AVCodecContext *avctx) {

    int ret = JS_ERR;

    JNIEnv *env = NULL;
    struct JNIMediaCodecListFields jfields = {0};
    jfieldID field_id = 0;

    JNI_GET_ENV_OR_RETURN(env, avctx, -1);

    if (js_jni_init_jfields(env, &jfields, jni_mediacodeclist_mapping, 0, avctx) < 0) {
        goto done;
    }

    if (avctx->codec_id == AV_CODEC_ID_H264) {
        switch (avctx->profile) {
            case FF_PROFILE_H264_BASELINE:
            case FF_PROFILE_H264_CONSTRAINED_BASELINE:
                field_id = jfields.avc_profile_baseline_id;
                break;
            case FF_PROFILE_H264_MAIN:
                field_id = jfields.avc_profile_main_id;
                break;
            case FF_PROFILE_H264_EXTENDED:
                field_id = jfields.avc_profile_extended_id;
                break;
            case FF_PROFILE_H264_HIGH:
                field_id = jfields.avc_profile_high_id;
                break;
            case FF_PROFILE_H264_HIGH_10:
            case FF_PROFILE_H264_HIGH_10_INTRA:
                field_id = jfields.avc_profile_high10_id;
                break;
            case FF_PROFILE_H264_HIGH_422:
            case FF_PROFILE_H264_HIGH_422_INTRA:
                field_id = jfields.avc_profile_high422_id;
                break;
            case FF_PROFILE_H264_HIGH_444:
            case FF_PROFILE_H264_HIGH_444_INTRA:
            case FF_PROFILE_H264_HIGH_444_PREDICTIVE:
                field_id = jfields.avc_profile_high444_id;
                break;
        }

        strcpy(mime_type, JS_MIME_VIDEO_AVC);
    } else if (avctx->codec_id == AV_CODEC_ID_HEVC) {
        switch (avctx->profile) {
            case FF_PROFILE_HEVC_MAIN:
            case FF_PROFILE_HEVC_MAIN_STILL_PICTURE:
                field_id = jfields.hevc_profile_main_id;
                break;
            case FF_PROFILE_HEVC_MAIN_10:
                field_id = jfields.hevc_profile_main10_id;
                break;
        }
        strcpy(mime_type, JS_MIME_VIDEO_HEVC);
    }


    if (field_id) {
        *profile = (*env)->GetStaticIntField(env, jfields.codec_profile_level_class, field_id);
        if (js_jni_exception_check(env, 1, avctx) < 0) {
            goto done;
        } else {
            *width = avctx->width;
            *height = avctx->height;
            *level = avctx->level;
            ret = JS_OK;
        }
    }

    done:
    js_jni_reset_jfields(env, &jfields, jni_mediacodeclist_mapping, 0, avctx);
    return ret;
}

char *
js_MediaCodecList_getCodecNameByType(const char *mime, int profile, int encoder, void *log_ctx) {
    int ret;
    int i;
    int codec_count;
    int found_codec = 0;
    char *name = NULL;
    char *supported_type = NULL;

    JNIEnv *env = NULL;
    struct JNIMediaCodecListFields jfields = {0};
    struct JNIMediaFormatFields mediaformat_jfields = {0};

    jobject format = NULL;
    jobject codec = NULL;
    jobject codec_name = NULL;

    jobject info = NULL;
    jobject type = NULL;
    jobjectArray types = NULL;

    jobject capabilities = NULL;
    jobject profile_level = NULL;
    jobjectArray profile_levels = NULL;

    JNI_GET_ENV_OR_RETURN(env, log_ctx, NULL);

    if ((ret = js_jni_init_jfields(env, &jfields, jni_mediacodeclist_mapping, 0, log_ctx)) < 0) {
        goto done;
    }

    if ((ret = js_jni_init_jfields(env, &mediaformat_jfields, jni_mediaformat_mapping, 0,
                                   log_ctx)) < 0) {
        goto done;
    }

    codec_count = (*env)->CallStaticIntMethod(env, jfields.mediacodec_list_class,
                                              jfields.get_codec_count_id);
    if (js_jni_exception_check(env, 1, log_ctx) < 0) {
        goto done;
    }

    for (i = 0; i < codec_count; i++) {
        int j;
        int type_count;
        int is_encoder;

        info = (*env)->CallStaticObjectMethod(env, jfields.mediacodec_list_class,
                                              jfields.get_codec_info_at_id, i);
        if (js_jni_exception_check(env, 1, log_ctx) < 0) {
            goto done;
        }

        types = (*env)->CallObjectMethod(env, info, jfields.get_supported_types_id);
        if (js_jni_exception_check(env, 1, log_ctx) < 0) {
            goto done;
        }

        is_encoder = (*env)->CallBooleanMethod(env, info, jfields.is_encoder_id);
        if (js_jni_exception_check(env, 1, log_ctx) < 0) {
            goto done;
        }

        if (is_encoder != encoder) {
            goto done_with_info;
        }

        type_count = (*env)->GetArrayLength(env, types);
        for (j = 0; j < type_count; j++) {
            int k;
            int profile_count;

            type = (*env)->GetObjectArrayElement(env, types, j);
            if (js_jni_exception_check(env, 1, log_ctx) < 0) {
                goto done;
            }

            supported_type = js_jni_jstring_to_utf_chars(env, type, log_ctx);
            if (!supported_type) {
                goto done;
            }

            if (!av_strcasecmp(supported_type, mime)) {
                codec_name = (*env)->CallObjectMethod(env, info, jfields.get_name_id);
                if (js_jni_exception_check(env, 1, log_ctx) < 0) {
                    goto done;
                }

                name = js_jni_jstring_to_utf_chars(env, codec_name, log_ctx);
                if (!name) {
                    goto done;
                }

                if (strstr(name, "OMX.google")) {
                    av_freep(&name);
                    goto done_with_type;
                }

                capabilities = (*env)->CallObjectMethod(env, info,
                                                        jfields.get_codec_capabilities_id, type);
                if (js_jni_exception_check(env, 1, log_ctx) < 0) {
                    goto done;
                }


                jintArray jint_arr = (jintArray) (*env)->GetObjectField(env, capabilities,
                                                                        jfields.color_formats_id);
                //获取arrays对象的指针
                jint *int_arr = (*env)->GetIntArrayElements(env, jint_arr, NULL);
                //获取数组的长度
                jsize len = (*env)->GetArrayLength(env, jint_arr);

                for (int i = 0; i < len; i++) {
                    LOGE("likang color=0x%x , %d", int_arr[i], int_arr[i]);
                }

                if (jint_arr) {
                    (*env)->DeleteLocalRef(env, jint_arr);
                }


                profile_levels = (*env)->GetObjectField(env, capabilities,
                                                        jfields.profile_levels_id);
                if (js_jni_exception_check(env, 1, log_ctx) < 0) {
                    goto done;
                }

                profile_count = (*env)->GetArrayLength(env, profile_levels);
                if (!profile_count) {
                    found_codec = 1;
                }
                for (k = 0; k < profile_count; k++) {
                    int supported_profile = 0;

                    if (profile < 0) {
                        found_codec = 1;
                        break;
                    }

                    profile_level = (*env)->GetObjectArrayElement(env, profile_levels, k);
                    if (js_jni_exception_check(env, 1, log_ctx) < 0) {
                        goto done;
                    }

                    supported_profile = (*env)->GetIntField(env, profile_level,
                                                            jfields.profile_id);
                    if (js_jni_exception_check(env, 1, log_ctx) < 0) {
                        goto done;
                    }

                    found_codec = profile == supported_profile;

                    if (profile_level) {
                        (*env)->DeleteLocalRef(env, profile_level);
                        profile_level = NULL;
                    }

                    if (found_codec) {
                        break;
                    }
                }
            }

            done_with_type:
            if (profile_levels) {
                (*env)->DeleteLocalRef(env, profile_levels);
                profile_levels = NULL;
            }

            if (capabilities) {
                (*env)->DeleteLocalRef(env, capabilities);
                capabilities = NULL;
            }

            if (type) {
                (*env)->DeleteLocalRef(env, type);
                type = NULL;
            }

            av_freep(&supported_type);

            if (found_codec) {
                break;
            }

            av_freep(&name);
        }

        done_with_info:
        if (info) {
            (*env)->DeleteLocalRef(env, info);
            info = NULL;
        }

        if (types) {
            (*env)->DeleteLocalRef(env, types);
            types = NULL;
        }

        if (found_codec) {
            break;
        }
    }

    done:
    if (format) {
        (*env)->DeleteLocalRef(env, format);
    }

    if (codec) {
        (*env)->DeleteLocalRef(env, codec);
    }

    if (codec_name) {
        (*env)->DeleteLocalRef(env, codec_name);
    }

    if (info) {
        (*env)->DeleteLocalRef(env, info);
    }

    if (type) {
        (*env)->DeleteLocalRef(env, type);
    }

    if (types) {
        (*env)->DeleteLocalRef(env, types);
    }

    if (capabilities) {
        (*env)->DeleteLocalRef(env, capabilities);
    }

    if (profile_level) {
        (*env)->DeleteLocalRef(env, profile_level);
    }

    if (profile_levels) {
        (*env)->DeleteLocalRef(env, profile_levels);
    }

    av_freep(&supported_type);

    js_jni_reset_jfields(env, &jfields, jni_mediacodeclist_mapping, 0, log_ctx);
    js_jni_reset_jfields(env, &mediaformat_jfields, jni_mediaformat_mapping, 0, log_ctx);

    if (!found_codec) {
        av_freep(&name);
    }

    return name;
}

void *js_MediaFormat_new(void) {

    if (__JS_NDK_MEDIACODEC_LINKED__) {
        return AMediaFormat_new();
    } else {

        JNIEnv *env = NULL;
        JSMediaFormat *format = NULL;
        jobject object = NULL;

        format = av_mallocz(sizeof(JSMediaFormat));
        if (!format) {
            return NULL;
        }
        format->
                class = &mediaformat_class;

        env = js_jni_get_env(format);
        if (!env) {
            av_freep(&format);
            return NULL;
        }

        if (js_jni_init_jfields(env, &format->jfields, jni_mediaformat_mapping, 1, format) < 0) {
            goto fail;
        }

        object = (*env)->NewObject(env, format->jfields.mediaformat_class, format->jfields.init_id);
        if (!object) {
            goto fail;
        }

        format->object = (*env)->NewGlobalRef(env, object);
        if (!format->object) {
            goto fail;
        }

        fail:
        if (object) {
            (*env)->DeleteLocalRef(env, object);
        }

        if (!format->object) {
            js_jni_reset_jfields(env, &format->jfields, jni_mediaformat_mapping, 1, format);
            av_freep(&format);
        }

        return format;
    }
}

static JSMediaFormat *js_MediaFormat_newFromObject(void *object) {
    JNIEnv *env = NULL;
    JSMediaFormat *format = NULL;

    format = av_mallocz(sizeof(JSMediaFormat));
    if (!format) {
        return NULL;
    }
    format->
            class = &mediaformat_class;

    env = js_jni_get_env(format);
    if (!env) {
        av_freep(&format);
        return NULL;
    }

    if (js_jni_init_jfields(env, &format->jfields, jni_mediaformat_mapping, 1, format) < 0) {
        goto fail;
    }

    format->object = (*env)->NewGlobalRef(env, object);
    if (!format->object) {
        goto fail;
    }

    return format;
    fail:
    js_jni_reset_jfields(env, &format->jfields, jni_mediaformat_mapping, 1, format);

    av_freep(&format);

    return NULL;
}

int js_MediaFormat_delete(void *pformat) {
    if (!pformat) {
        return 0;
    }
    if (__JS_NDK_MEDIACODEC_LINKED__) {
        return AMediaFormat_delete(pformat);
    } else {
        int ret = 0;
        JNIEnv *env = NULL;
        JSMediaFormat *format = pformat;
        JNI_GET_ENV_OR_RETURN(env, format, AVERROR_EXTERNAL);

        (*env)->DeleteGlobalRef(env, format->object);
        format->object = NULL;

        js_jni_reset_jfields(env, &format->jfields, jni_mediaformat_mapping, 1, format);

        av_freep(&format);
        return ret;
    }
}

char *js_MediaFormat_toString(void *pformat) {

    av_assert0(pformat != NULL);
    if (__JS_NDK_MEDIACODEC_LINKED__) {
        return (char *) AMediaFormat_toString(pformat);

    } else {
        char *ret = NULL;

        JNIEnv *env = NULL;
        jstring description = NULL;
        JSMediaFormat *format = pformat;

        JNI_GET_ENV_OR_RETURN(env, format, NULL);

        description = (*env)->CallObjectMethod(env, format->object, format->jfields.to_string_id);
        if (js_jni_exception_check(env, 1, NULL) < 0) {
            goto fail;
        }

        ret = js_jni_jstring_to_utf_chars(env, description, format);
        fail:
        if (description) {
            (*env)->DeleteLocalRef(env, description);
        }

        return ret;

    }
}

int js_MediaFormat_getInt32(void *pformat, const char *name, int32_t *out) {

    av_assert0(pformat != NULL);

    if (__JS_NDK_MEDIACODEC_LINKED__) {

        return AMediaFormat_getInt32(pformat, name, out);

    } else {

        int ret = 1;
        JNIEnv *env = NULL;
        jstring key = NULL;
        JSMediaFormat *format = pformat;

        JNI_GET_ENV_OR_RETURN(env, format, 0);

        key = js_jni_utf_chars_to_jstring(env, name, format);
        if (!key) {
            ret = 0;
            goto fail;
        }

        *out = (*env)->CallIntMethod(env, format->object, format->jfields.get_integer_id, key);
        if ((ret = js_jni_exception_check(env, 1, format)) < 0) {
            ret = 0;
            goto fail;
        }

        ret = 1;
        fail:
        if (key) {
            (*env)->DeleteLocalRef(env, key);
        }

        return ret;
    }
}

int js_MediaFormat_getInt64(void *pformat, const char *name, int64_t *out) {
    av_assert0(pformat != NULL);

    if (__JS_NDK_MEDIACODEC_LINKED__) {

        return AMediaFormat_getInt64(pformat, name, out);

    } else {
        int ret = 1;

        JNIEnv *env = NULL;
        jstring key = NULL;
        JSMediaFormat *format = pformat;

        JNI_GET_ENV_OR_RETURN(env, format, 0);

        key = js_jni_utf_chars_to_jstring(env, name, format);
        if (!key) {
            ret = 0;
            goto fail;
        }

        *out = (*env)->CallLongMethod(env, format->object, format->jfields.get_long_id, key);
        if ((ret = js_jni_exception_check(env, 1, format)) < 0) {
            ret = 0;
            goto fail;
        }

        ret = 1;
        fail:
        if (key) {
            (*env)->DeleteLocalRef(env, key);
        }

        return ret;
    }
}

int js_MediaFormat_getFloat(void *pformat, const char *name, float *out) {
    av_assert0(pformat != NULL);

    if (__JS_NDK_MEDIACODEC_LINKED__) {

        return AMediaFormat_getFloat(pformat, name, out);

    } else {
        int ret = 1;

        JNIEnv *env = NULL;
        jstring key = NULL;
        JSMediaFormat *format = pformat;

        JNI_GET_ENV_OR_RETURN(env, format, 0);

        key = js_jni_utf_chars_to_jstring(env, name, format);
        if (!key) {
            ret = 0;
            goto fail;
        }

        *out = (*env)->CallFloatMethod(env, format->object, format->jfields.get_float_id, key);
        if ((ret = js_jni_exception_check(env, 1, format)) < 0) {
            ret = 0;
            goto fail;
        }

        ret = 1;
        fail:
        if (key) {
            (*env)->DeleteLocalRef(env, key);
        }

        return ret;
    }
}

int
js_MediaFormat_getBuffer(void *pformat, const char *name, void **data, size_t *size) {
    av_assert0(pformat != NULL);

    if (__JS_NDK_MEDIACODEC_LINKED__) {

        return AMediaFormat_getBuffer(pformat, name, data, size);

    } else {
        int ret = 1;

        JNIEnv *env = NULL;
        jstring key = NULL;
        jobject result = NULL;
        JSMediaFormat *format = pformat;

        JNI_GET_ENV_OR_RETURN(env, format, 0);

        key = js_jni_utf_chars_to_jstring(env, name, format);
        if (!key) {
            ret = 0;
            goto fail;
        }

        result = (*env)->CallObjectMethod(env, format->object, format->jfields.get_bytebuffer_id,
                                          key);
        if ((ret = js_jni_exception_check(env, 1, format)) < 0) {
            ret = 0;
            goto fail;
        }

        *data = (*env)->GetDirectBufferAddress(env, result);
        *size = (*env)->GetDirectBufferCapacity(env, result);

        if (*data && *size) {
            void *src = *data;
            *data = av_malloc(*size);
            if (!*data) {
                ret = 0;
                goto fail;
            }

            memcpy(*data, src, *size);
        }

        ret = 1;
        fail:
        if (key) {
            (*env)->DeleteLocalRef(env, key);
        }

        if (result) {
            (*env)->DeleteLocalRef(env, result);
        }

        return ret;
    }
}

int js_MediaFormat_getString(void *pformat, const char *name, const char **out) {
    av_assert0(pformat != NULL);

    if (__JS_NDK_MEDIACODEC_LINKED__) {

        return AMediaFormat_getString(pformat, name, out);

    } else {
        int ret = 1;

        JNIEnv *env = NULL;
        jstring key = NULL;
        jstring result = NULL;

        JSMediaFormat *format = pformat;

        JNI_GET_ENV_OR_RETURN(env, format, 0);

        key = js_jni_utf_chars_to_jstring(env, name, format);
        if (!key) {
            ret = 0;
            goto fail;
        }

        result = (*env)->CallObjectMethod(env, format->object, format->jfields.get_string_id, key);
        if ((ret = js_jni_exception_check(env, 1, format)) < 0) {
            ret = 0;
            goto fail;
        }

        *out = js_jni_jstring_to_utf_chars(env, result, format);
        if (!*out) {
            ret = 0;
            goto fail;
        }

        ret = 1;
        fail:
        if (key) {
            (*env)->DeleteLocalRef(env, key);
        }

        if (result) {
            (*env)->DeleteLocalRef(env, result);
        }

        return ret;
    }
}

void js_MediaFormat_setInt32(void *pformat, const char *name, int32_t value) {

    av_assert0(pformat != NULL);
    if (__JS_NDK_MEDIACODEC_LINKED__) {
        AMediaFormat_setInt32(pformat, name, value);
    } else {
        JNIEnv *env = NULL;
        jstring key = NULL;
        JSMediaFormat *format = pformat;
        JNI_GET_ENV_OR_RETURN_VOID(env, format);

        key = js_jni_utf_chars_to_jstring(env, name, format);
        if (!key) {
            goto fail;
        }

        (*env)->CallVoidMethod(env, format->object, format->jfields.set_integer_id, key, value);
        if (js_jni_exception_check(env, 1, format) < 0) {
            goto fail;
        }

        fail:
        if (key) {
            (*env)->DeleteLocalRef(env, key);
        }
    }
}

void js_MediaFormat_setInt64(void *pformat, const char *name, int64_t value) {
    av_assert0(pformat != NULL);
    if (__JS_NDK_MEDIACODEC_LINKED__) {
        AMediaFormat_setInt64(pformat, name, value);
    } else {
        JNIEnv *env = NULL;
        jstring key = NULL;

        JSMediaFormat *format = pformat;

        JNI_GET_ENV_OR_RETURN_VOID(env, format);

        key = js_jni_utf_chars_to_jstring(env, name, format);
        if (!key) {
            goto fail;
        }

        (*env)->CallVoidMethod(env, format->object, format->jfields.set_long_id, key, value);
        if (js_jni_exception_check(env, 1, format) < 0) {
            goto fail;
        }

        fail:
        if (key) {
            (*env)->DeleteLocalRef(env, key);
        }
    }
}

void js_MediaFormat_setFloat(void *pformat, const char *name, float value) {
    av_assert0(pformat != NULL);
    if (__JS_NDK_MEDIACODEC_LINKED__) {
        AMediaFormat_setFloat(pformat, name, value);
    } else {
        JNIEnv *env = NULL;
        jstring key = NULL;

        JSMediaFormat *format = pformat;

        JNI_GET_ENV_OR_RETURN_VOID(env, format);

        key = js_jni_utf_chars_to_jstring(env, name, format);
        if (!key) {
            goto fail;
        }

        (*env)->CallVoidMethod(env, format->object, format->jfields.set_float_id, key, value);
        if (js_jni_exception_check(env, 1, format) < 0) {
            goto fail;
        }

        fail:
        if (key) {
            (*env)->DeleteLocalRef(env, key);
        }
    }
}

void js_MediaFormat_setString(void *pformat, const char *name, const char *value) {
    av_assert0(pformat != NULL);

    if (__JS_NDK_MEDIACODEC_LINKED__) {

        AMediaFormat_setString(pformat, name, value);

    } else {

        JNIEnv *env = NULL;
        jstring key = NULL;
        jstring string = NULL;
        JSMediaFormat *format = pformat;

        JNI_GET_ENV_OR_RETURN_VOID(env, format);

        key = js_jni_utf_chars_to_jstring(env, name, format);
        if (!key) {
            goto fail;
        }

        string = js_jni_utf_chars_to_jstring(env, value, format);
        if (!string) {
            goto fail;
        }

        (*env)->CallVoidMethod(env, format->object, format->jfields.set_string_id, key, string);
        if (js_jni_exception_check(env, 1, format) < 0) {
            goto fail;
        }

        fail:
        if (key) {
            (*env)->DeleteLocalRef(env, key);
        }

        if (string) {
            (*env)->DeleteLocalRef(env, string);
        }
    }
}

void
js_MediaFormat_setBuffer(void *pformat, const char *name, void *data, size_t size) {

    av_assert0(pformat != NULL);
    if (__JS_NDK_MEDIACODEC_LINKED__) {

        AMediaFormat_setBuffer(pformat, name, data, size);
    } else {

        JNIEnv *env = NULL;
        jstring key = NULL;
        jobject buffer = NULL;
        void *buffer_data = NULL;

        JSMediaFormat *format = pformat;
        JNI_GET_ENV_OR_RETURN_VOID(env, format);

        key = js_jni_utf_chars_to_jstring(env, name, format);
        if (!key) {
            goto fail;
        }

        if (!data || !size) {
            goto fail;
        }

        buffer_data = av_malloc(size);
        if (!buffer_data) {
            goto fail;
        }

        memcpy(buffer_data, data, size);

        buffer = (*env)->NewDirectByteBuffer(env, buffer_data, size);
        if (!buffer) {
            goto fail;
        }

        (*env)->CallVoidMethod(env, format->object, format->jfields.set_bytebuffer_id, key, buffer);
        if (js_jni_exception_check(env, 1, format) < 0) {
            goto fail;
        }

        fail:
        if (key) {
            (*env)->DeleteLocalRef(env, key);
        }

        if (buffer) {
            (*env)->DeleteLocalRef(env, buffer);
        }
    }
}

static int codec_init_static_fields(JSMediaCodec *codec) {
    int ret = 0;
    JNIEnv *env = NULL;

    JNI_GET_ENV_OR_RETURN(env, codec, AVERROR_EXTERNAL);

    codec->JS_INFO_TRY_AGAIN_LATER = (*env)->GetStaticIntField(env,
                                                               codec->jfields.mediacodec_class,
                                                               codec->jfields.info_try_again_later_id);
    if ((ret = js_jni_exception_check(env, 1, codec)) < 0) {
        goto fail;
    }

    codec->JS_BUFFER_FLAG_CODEC_CONFIG = (*env)->GetStaticIntField(env,
                                                                   codec->jfields.mediacodec_class,
                                                                   codec->jfields.buffer_flag_codec_config_id);
    if ((ret = js_jni_exception_check(env, 1, codec)) < 0) {
        goto fail;
    }

    codec->JS_BUFFER_FLAG_END_OF_STREAM = (*env)->GetStaticIntField(env,
                                                                    codec->jfields.mediacodec_class,
                                                                    codec->jfields.buffer_flag_end_of_stream_id);
    if ((ret = js_jni_exception_check(env, 1, codec)) < 0) {
        goto fail;
    }

    if (codec->jfields.buffer_flag_key_frame_id) {
        codec->JS_BUFFER_FLAG_KEY_FRAME = (*env)->GetStaticIntField(env,
                                                                    codec->jfields.mediacodec_class,
                                                                    codec->jfields.buffer_flag_key_frame_id);
        if ((ret = js_jni_exception_check(env, 1, codec)) < 0) {
            goto fail;
        }
    }

    codec->JS_CONFIGURE_FLAG_ENCODE = (*env)->GetStaticIntField(env,
                                                                codec->jfields.mediacodec_class,
                                                                codec->jfields.configure_flag_encode_id);
    if ((ret = js_jni_exception_check(env, 1, codec)) < 0) {
        goto fail;
    }

    codec->JS_INFO_TRY_AGAIN_LATER = (*env)->GetStaticIntField(env,
                                                               codec->jfields.mediacodec_class,
                                                               codec->jfields.info_try_again_later_id);
    if ((ret = js_jni_exception_check(env, 1, codec)) < 0) {
        goto fail;
    }

    codec->JS_INFO_OUTPUT_BUFFERS_CHANGED = (*env)->GetStaticIntField(env,
                                                                      codec->jfields.mediacodec_class,
                                                                      codec->jfields.info_output_buffers_changed_id);
    if ((ret = js_jni_exception_check(env, 1, codec)) < 0) {
        goto fail;
    }

    codec->JS_INFO_OUTPUT_FORMAT_CHANGED = (*env)->GetStaticIntField(env,
                                                                     codec->jfields.mediacodec_class,
                                                                     codec->jfields.info_output_format_changed_id);
    if ((ret = js_jni_exception_check(env, 1, codec)) < 0) {
        goto fail;
    }

    fail:

    return ret;
}

void *js_MediaCodec_createCodecByName(const char *name) {

    if (__JS_NDK_MEDIACODEC_LINKED__) {
        return AMediaCodec_createCodecByName(name);

    } else {

        int ret = -1;
        JNIEnv *env = NULL;
        JSMediaCodec *codec = NULL;
        jstring codec_name = NULL;
        jobject object = NULL;

        codec = av_mallocz(sizeof(JSMediaCodec));
        if (!codec) {
            return NULL;
        }
        codec->class = &mediacodec_class;

        env = js_jni_get_env(codec);
        if (!env) {
            av_freep(&codec);
            return NULL;
        }

        if (js_jni_init_jfields(env, &codec->jfields, jni_mediacodec_mapping, 1, codec) < 0) {
            goto fail;
        }

        codec_name = js_jni_utf_chars_to_jstring(env, name, codec);
        if (!codec_name) {
            goto fail;
        }

        object = (*env)->CallStaticObjectMethod(env, codec->jfields.mediacodec_class,
                                                codec->jfields.create_by_codec_name_id, codec_name);
        if (js_jni_exception_check(env, 1, codec) < 0) {
            goto fail;
        }

        codec->object = (*env)->NewGlobalRef(env, object);
        if (!codec->object) {
            goto fail;
        }

        if (codec_init_static_fields(codec) < 0) {
            goto fail;
        }

        if (codec->jfields.get_input_buffer_id && codec->jfields.get_output_buffer_id) {
            codec->has_get_i_o_buffer = 1;
        }

        ret = 0;
        fail:
        if (codec_name) {
            (*env)->DeleteLocalRef(env, codec_name);
        }

        if (object) {
            (*env)->DeleteLocalRef(env, object);
        }

        if (ret < 0) {
            js_jni_reset_jfields(env, &codec->jfields, jni_mediacodec_mapping, 1, codec);
            av_freep(&codec);
        }

        return codec;
    }
}

void *js_MediaCodec_createDecoderByType(const char *mime) {
    if (__JS_NDK_MEDIACODEC_LINKED__) {
        return AMediaCodec_createDecoderByType(mime);

    } else {

        int ret = -1;
        JNIEnv *env = NULL;
        JSMediaCodec *codec = NULL;
        jstring mime_type = NULL;
        jobject object = NULL;

        codec = av_mallocz(sizeof(JSMediaCodec));
        if (!codec) {
            return NULL;
        }
        codec->class = &mediacodec_class;

        env = js_jni_get_env(codec);
        if (!env) {
            av_freep(&codec);
            return NULL;
        }

        if (js_jni_init_jfields(env, &codec->jfields, jni_mediacodec_mapping, 1, codec) < 0) {
            goto fail;
        }

        mime_type = js_jni_utf_chars_to_jstring(env, mime, codec);
        if (!mime_type) {
            goto fail;
        }

        object = (*env)->CallStaticObjectMethod(env, codec->jfields.mediacodec_class,
                                                codec->jfields.create_decoder_by_type_id,
                                                mime_type);
        if (js_jni_exception_check(env, 1, codec) < 0) {
            goto fail;
        }

        codec->object = (*env)->NewGlobalRef(env, object);
        if (!codec->object) {
            goto fail;
        }

        if (codec_init_static_fields(codec) < 0) {
            goto fail;
        }

        if (codec->jfields.get_input_buffer_id && codec->jfields.get_output_buffer_id) {
            codec->has_get_i_o_buffer = 1;
        }

        ret = 0;
        fail:
        if (mime_type) {
            (*env)->DeleteLocalRef(env, mime_type);
        }

        if (object) {
            (*env)->DeleteLocalRef(env, object);
        }

        if (ret < 0) {
            js_jni_reset_jfields(env, &codec->jfields, jni_mediacodec_mapping, 1, codec);
            av_freep(&codec);
        }

        return codec;
    }
}

void *js_MediaCodec_createEncoderByType(const char *mime) {
    if (__JS_NDK_MEDIACODEC_LINKED__) {
        return AMediaCodec_createEncoderByType(mime);

    } else {
        int ret = -1;
        JNIEnv *env = NULL;
        JSMediaCodec *codec = NULL;
        jstring mime_type = NULL;
        jobject object = NULL;

        codec = av_mallocz(sizeof(JSMediaCodec));
        if (!codec) {
            return NULL;
        }
        codec->
                class = &mediacodec_class;

        env = js_jni_get_env(codec);
        if (!env) {
            av_freep(&codec);
            return NULL;
        }

        if (js_jni_init_jfields(env, &codec->jfields, jni_mediacodec_mapping, 1, codec) < 0) {
            goto fail;
        }

        mime_type = js_jni_utf_chars_to_jstring(env, mime, codec);
        if (!mime_type) {
            goto fail;
        }

        object = (*env)->CallStaticObjectMethod(env, codec->jfields.mediacodec_class,
                                                codec->jfields.create_encoder_by_type_id,
                                                mime_type);
        if (js_jni_exception_check(env, 1, codec) < 0) {
            goto fail;
        }

        codec->object = (*env)->NewGlobalRef(env, object);
        if (!codec->object) {
            goto fail;
        }

        if (codec_init_static_fields(codec) < 0) {
            goto fail;
        }

        if (codec->jfields.get_input_buffer_id && codec->jfields.get_output_buffer_id) {
            codec->has_get_i_o_buffer = 1;
        }

        ret = 0;
        fail:
        if (mime_type) {
            (*env)->DeleteLocalRef(env, mime_type);
        }

        if (object) {
            (*env)->DeleteLocalRef(env, object);
        }

        if (ret < 0) {
            js_jni_reset_jfields(env, &codec->jfields, jni_mediacodec_mapping, 1, codec);
            av_freep(&codec);
        }

        return codec;
    }
}

int js_MediaCodec_delete(void *pcodec) {

    if (!pcodec) {
        return 0;
    }
    if (__JS_NDK_MEDIACODEC_LINKED__) {

        return AMediaCodec_delete(pcodec);

    } else {
        int ret = 0;
        JNIEnv *env = NULL;
        JSMediaCodec *codec = pcodec;
        JNI_GET_ENV_OR_RETURN(env, codec, AVERROR_EXTERNAL);
        (*env)->CallVoidMethod(env, codec->object, codec->jfields.release_id);
        if (js_jni_exception_check(env, 1, codec) < 0) {
            ret = AVERROR_EXTERNAL;
        }

        (*env)->DeleteGlobalRef(env, codec->object);
        codec->object = NULL;

        js_jni_reset_jfields(env, &codec->jfields, jni_mediacodec_mapping, 1, codec);

        av_freep(&codec);
        return ret;
    }

}

char *js_MediaCodec_getName(JSMediaCodec *codec) {
    char *ret = NULL;
    JNIEnv *env = NULL;
    jobject *name = NULL;

    JNI_GET_ENV_OR_RETURN(env, codec, NULL);

    name = (*env)->CallObjectMethod(env, codec->object, codec->jfields.get_name_id);
    if (js_jni_exception_check(env, 1, codec) < 0) {
        goto fail;
    }

    ret = js_jni_jstring_to_utf_chars(env, name, codec);

    fail:
    return ret;
}

int js_MediaCodec_configure(void *pcodec, const void *pformat, void *surface,
                            void *crypto, uint32_t flags) {

    if (__JS_NDK_MEDIACODEC_LINKED__) {
        return AMediaCodec_configure(pcodec, pformat, surface, crypto, flags);
    } else {
        int ret = 0;
        JNIEnv *env = NULL;
        JSMediaCodec *codec = pcodec;
        const JSMediaFormat *format = pformat;
        JNI_GET_ENV_OR_RETURN(env, codec, AVERROR_EXTERNAL);

        (*env)->CallVoidMethod(env, codec->object, codec->jfields.configure_id, format->object,
                               surface,
                               NULL, flags);
        if (js_jni_exception_check(env, 1, codec) < 0) {
            ret = AVERROR_EXTERNAL;
            goto fail;
        }

        fail:
        return ret;
    }

}

int js_MediaCodec_start(void *pcodec) {

    if (__JS_NDK_MEDIACODEC_LINKED__) {
        return AMediaCodec_start(pcodec);

    } else {
        int ret = 0;
        JNIEnv *env = NULL;
        JSMediaCodec *codec = pcodec;
        JNI_GET_ENV_OR_RETURN(env, codec, AVERROR_EXTERNAL);
        (*env)->CallVoidMethod(env, codec->object, codec->jfields.start_id);
        if (js_jni_exception_check(env, 1, codec) < 0) {
            ret = AVERROR_EXTERNAL;
            goto fail;
        }

        fail:
        return ret;
    }
}

int js_MediaCodec_stop(void *pcodec) {
    if (__JS_NDK_MEDIACODEC_LINKED__) {
        return AMediaCodec_stop(pcodec);
    } else {
        int ret = 0;
        JNIEnv *env = NULL;
        JSMediaCodec *codec = pcodec;
        JNI_GET_ENV_OR_RETURN(env, codec, AVERROR_EXTERNAL);

        (*env)->CallVoidMethod(env, codec->object, codec->jfields.stop_id);
        if (js_jni_exception_check(env, 1, codec) < 0) {
            ret = AVERROR_EXTERNAL;
            goto fail;
        }

        fail:
        return ret;
    }
}

int js_MediaCodec_flush(void *pcodec) {

    if (__JS_NDK_MEDIACODEC_LINKED__) {
        return AMediaCodec_flush(pcodec);
    } else {
        int ret = 0;
        JNIEnv *env = NULL;
        JSMediaCodec *codec = pcodec;
        JNI_GET_ENV_OR_RETURN(env, codec, AVERROR_EXTERNAL);

        (*env)->CallVoidMethod(env, codec->object, codec->jfields.flush_id);
        if (js_jni_exception_check(env, 1, codec) < 0) {
            ret = AVERROR_EXTERNAL;
            goto fail;
        }

        fail:
        return ret;
    }
}

int js_MediaCodec_releaseOutputBuffer(void *pcodec, size_t idx, int render) {
    if (__JS_NDK_MEDIACODEC_LINKED__) {
        return AMediaCodec_releaseOutputBuffer(pcodec, idx, render);
    } else {

        int ret = 0;
        JNIEnv *env = NULL;
        JSMediaCodec *codec = pcodec;
        JNI_GET_ENV_OR_RETURN(env, codec, AVERROR_EXTERNAL);

        (*env)->CallVoidMethod(env, codec->object, codec->jfields.release_output_buffer_id,
                               (jint) idx,
                               (jboolean) render);
        if (js_jni_exception_check(env, 1, codec) < 0) {
            ret = AVERROR_EXTERNAL;
            goto fail;
        }

        fail:
        return ret;
    }
}

int js_MediaCodec_releaseOutputBufferAtTime(void *pcodec, size_t idx,
                                            int64_t timestampNs) {
    if (__JS_NDK_MEDIACODEC_LINKED__) {
        return AMediaCodec_releaseOutputBufferAtTime(pcodec, idx, timestampNs);
    } else {

        int ret = 0;
        JNIEnv *env = NULL;
        JSMediaCodec *codec = pcodec;
        JNI_GET_ENV_OR_RETURN(env, codec, AVERROR_EXTERNAL);

        (*env)->CallVoidMethod(env, codec->object, codec->jfields.release_output_buffer_at_time_id,
                               (jint) idx, timestampNs);
        if (js_jni_exception_check(env, 1, codec) < 0) {
            ret = AVERROR_EXTERNAL;
            goto fail;
        }

        fail:
        return ret;
    }
}

ssize_t js_MediaCodec_dequeueInputBuffer(void *pcodec, int64_t timeoutUs) {

    if (__JS_NDK_MEDIACODEC_LINKED__) {
        return AMediaCodec_dequeueInputBuffer(pcodec,
                                              timeoutUs);
    } else {
        int ret = 0;
        JNIEnv *env = NULL;
        JSMediaCodec *codec = pcodec;
        JNI_GET_ENV_OR_RETURN(env, codec, AVERROR_EXTERNAL);

        ret = (*env)->CallIntMethod(env, codec->object, codec->jfields.dequeue_input_buffer_id,
                                    timeoutUs);
        if (js_jni_exception_check(env, 1, codec) < 0) {
            ret = AVERROR_EXTERNAL;
            goto fail;
        }

        fail:
        return ret;
    }
}

int js_MediaCodec_queueInputBuffer(void *pcodec, size_t idx, off_t offset, size_t size,
                                   uint64_t time, uint32_t flags) {

    if (__JS_NDK_MEDIACODEC_LINKED__) {

        return AMediaCodec_queueInputBuffer(pcodec, idx, offset, size,
                                            time, flags);
    } else {

        int ret = 0;
        JNIEnv *env = NULL;
        JSMediaCodec *codec = pcodec;
        JNI_GET_ENV_OR_RETURN(env, codec, AVERROR_EXTERNAL);

        (*env)->CallVoidMethod(env, codec->object, codec->jfields.queue_input_buffer_id, (jint) idx,
                               (jint) offset, (jint) size, time, flags);
        if ((ret = js_jni_exception_check(env, 1, codec)) < 0) {
            ret = AVERROR_EXTERNAL;
            goto fail;
        }

        fail:
        return ret;
    }
}

ssize_t js_MediaCodec_dequeueOutputBuffer(void *pcodec, JSMediaCodecBufferInfo *info,
                                          int64_t timeoutUs) {
    if (__JS_NDK_MEDIACODEC_LINKED__) {

        return AMediaCodec_dequeueOutputBuffer(pcodec, info,
                                               timeoutUs);
    } else {
        int ret = 0;
        JNIEnv *env = NULL;
        JSMediaCodec *codec = pcodec;
        jobject mediainfo = NULL;

        JNI_GET_ENV_OR_RETURN(env, codec, AVERROR_EXTERNAL);

        mediainfo = (*env)->NewObject(env, codec->jfields.mediainfo_class, codec->jfields.init_id);
        if (js_jni_exception_check(env, 1, codec) < 0) {
            ret = AVERROR_EXTERNAL;
            goto fail;
        }

        ret = (*env)->CallIntMethod(env, codec->object, codec->jfields.dequeue_output_buffer_id,
                                    mediainfo, timeoutUs);
        if (js_jni_exception_check(env, 1, codec) < 0) {
            ret = AVERROR_EXTERNAL;
            goto fail;
        }

        info->flags = (*env)->GetIntField(env, mediainfo, codec->jfields.flags_id);
        if (js_jni_exception_check(env, 1, codec) < 0) {
            ret = AVERROR_EXTERNAL;
            goto fail;
        }

        info->offset = (*env)->GetIntField(env, mediainfo, codec->jfields.offset_id);
        if (js_jni_exception_check(env, 1, codec) < 0) {
            ret = AVERROR_EXTERNAL;
            goto fail;
        }

        info->presentationTimeUs = (*env)->GetLongField(env, mediainfo,
                                                        codec->jfields.presentation_time_us_id);
        if (js_jni_exception_check(env, 1, codec) < 0) {
            ret = AVERROR_EXTERNAL;
            goto fail;
        }

        info->size = (*env)->GetIntField(env, mediainfo, codec->jfields.size_id);
        if (js_jni_exception_check(env, 1, codec) < 0) {
            ret = AVERROR_EXTERNAL;
            goto fail;
        }
        fail:
        if (mediainfo) {
            (*env)->DeleteLocalRef(env, mediainfo);
        }

        return ret;
    }
}

uint8_t *js_MediaCodec_getInputBuffer(void *pcodec, size_t idx, size_t *out_size) {

    if (__JS_NDK_MEDIACODEC_LINKED__) {
        return AMediaCodec_getInputBuffer(pcodec, idx, out_size);
    } else {

        uint8_t *ret = NULL;
        JNIEnv *env = NULL;

        jobject buffer = NULL;
        jobject input_buffers = NULL;
        JSMediaCodec *codec = pcodec;

        JNI_GET_ENV_OR_RETURN(env, codec, NULL);

        if (codec->has_get_i_o_buffer) {
            buffer = (*env)->CallObjectMethod(env, codec->object,
                                              codec->jfields.get_input_buffer_id,
                                              (jint) idx);
            if (js_jni_exception_check(env, 1, codec) < 0) {
                goto fail;
            }
        } else {
            if (!codec->input_buffers) {
                input_buffers = (*env)->CallObjectMethod(env, codec->object,
                                                         codec->jfields.get_input_buffers_id);
                if (js_jni_exception_check(env, 1, codec) < 0) {
                    goto fail;
                }

                codec->input_buffers = (*env)->NewGlobalRef(env, input_buffers);
                if (js_jni_exception_check(env, 1, codec) < 0) {
                    goto fail;
                }
            }

            buffer = (*env)->GetObjectArrayElement(env, codec->input_buffers, idx);
            if (js_jni_exception_check(env, 1, codec) < 0) {
                goto fail;
            }
        }

        ret = (*env)->GetDirectBufferAddress(env, buffer);
        *out_size = (*env)->GetDirectBufferCapacity(env, buffer);
        fail:
        if (buffer) {
            (*env)->DeleteLocalRef(env, buffer);
        }

        if (input_buffers) {
            (*env)->DeleteLocalRef(env, input_buffers);
        }

        return ret;
    }
}

uint8_t *js_MediaCodec_getOutputBuffer(void *pcodec, size_t idx, size_t *out_size) {
    if (__JS_NDK_MEDIACODEC_LINKED__) {
        return AMediaCodec_getOutputBuffer(pcodec, idx, out_size);
    } else {

        uint8_t *ret = NULL;
        JNIEnv *env = NULL;

        jobject buffer = NULL;
        jobject output_buffers = NULL;
        JSMediaCodec *codec = pcodec;
        JNI_GET_ENV_OR_RETURN(env, codec, NULL);

        if (codec->has_get_i_o_buffer) {
            buffer = (*env)->CallObjectMethod(env, codec->object,
                                              codec->jfields.get_output_buffer_id,
                                              (jint) idx);
            if (js_jni_exception_check(env, 1, codec) < 0) {
                goto fail;
            }
        } else {
            if (!codec->output_buffers) {
                output_buffers = (*env)->CallObjectMethod(env, codec->object,
                                                          codec->jfields.get_output_buffers_id);
                if (js_jni_exception_check(env, 1, codec) < 0) {
                    goto fail;
                }

                codec->output_buffers = (*env)->NewGlobalRef(env, output_buffers);
                if (js_jni_exception_check(env, 1, codec) < 0) {
                    goto fail;
                }
            }

            buffer = (*env)->GetObjectArrayElement(env, codec->output_buffers, idx);
            if (js_jni_exception_check(env, 1, codec) < 0) {
                goto fail;
            }
        }

        ret = (*env)->GetDirectBufferAddress(env, buffer);
        *out_size = (*env)->GetDirectBufferCapacity(env, buffer);
        fail:
        if (buffer) {
            (*env)->DeleteLocalRef(env, buffer);
        }

        if (output_buffers) {
            (*env)->DeleteLocalRef(env, output_buffers);
        }

        return ret;

    }
}

void *js_MediaCodec_getOutputFormat(void *pcodec) {

    if (__JS_NDK_MEDIACODEC_LINKED__) {
        return AMediaCodec_getOutputFormat(pcodec);
    } else {

        JSMediaFormat *ret = NULL;
        JSMediaCodec *codec = pcodec;
        JNIEnv *env = NULL;
        jobject mediaformat = NULL;

        JNI_GET_ENV_OR_RETURN(env, codec, NULL);

        mediaformat = (*env)->CallObjectMethod(env, codec->object,
                                               codec->jfields.get_output_format_id);
        if (js_jni_exception_check(env, 1, codec) < 0) {
            goto fail;
        }

        ret = js_MediaFormat_newFromObject(mediaformat);
        fail:
        if (mediaformat) {
            (*env)->DeleteLocalRef(env, mediaformat);
        }

        return ret;
    }
}

int js_MediaCodec_getInfoTryAgainLater(JSMediaCodec *codec) {
    return codec->JS_INFO_TRY_AGAIN_LATER;
}

int js_MediaCodec_getInfoOutputBuffersChanged(JSMediaCodec *codec) {
    return codec->JS_INFO_OUTPUT_BUFFERS_CHANGED;
}

int js_MediaCodec_getInfoOutputFormatChanged(JSMediaCodec *codec) {
    return codec->JS_INFO_OUTPUT_FORMAT_CHANGED;
}


int js_MediaCodec_getBufferFlagCodecConfig(JSMediaCodec *codec) {
    return codec->JS_BUFFER_FLAG_CODEC_CONFIG;
}

int js_MediaCodec_getBufferFlagEndOfStream(JSMediaCodec *codec) {
    return codec->JS_BUFFER_FLAG_END_OF_STREAM;
}

int js_MediaCodec_getBufferFlagKeyFrame(JSMediaCodec *codec) {
    return codec->JS_BUFFER_FLAG_KEY_FRAME;
}

int js_MediaCodec_getConfigureFlagEncode(JSMediaCodec *codec) {
    return codec->JS_CONFIGURE_FLAG_ENCODE;
}

int js_MediaCodec_cleanOutputBuffers(JSMediaCodec *codec) {
    int ret = 0;

    if (!codec->has_get_i_o_buffer) {
        if (codec->output_buffers) {
            JNIEnv *env = NULL;

            env = js_jni_get_env(codec);
            if (!env) {
                ret = AVERROR_EXTERNAL;
                goto fail;
            }

            (*env)->DeleteGlobalRef(env, codec->output_buffers);
            codec->output_buffers = NULL;
        }
    }

    fail:
    return ret;
}
