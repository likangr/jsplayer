/**
 *  Android MediaCodec MPEG-2 / H.264 / H.265 / MPEG-4 / VP8 / VP9 decoders
 */

#include "js_media_decoder.h"
#include "js_mediacodec_def.h"
#include "converter/js_media_converter.h"
#include "js_constant.h"

extern "C" {
#include "js_ndk_mediacodec_proxy.h"
#include "js_java_mediacodec_wrapper.h"
#include "util/js_log.h"
#include "util/js_jni.h"
}

#define INPUT_DEQUEUE_TIMEOUT_US        8*1000
#define OUTPUT_DEQUEUE_TIMEOUT_US       8*1000

JSMediaDecoder::JSMediaDecoder(JSEventHandler *js_eventHandler) {
    m_js_event_handler = js_eventHandler;
}

JSMediaDecoder::~JSMediaDecoder() {

    if (m_video_hw_dec_ctx) {
        delete m_video_hw_dec_ctx;
    }
    if (m_video_hw_dec) {
        js_MediaCodec_stop(m_video_hw_dec);
        js_MediaCodec_delete(m_video_hw_dec);
    }
    if (m_video_sw_dec_ctx) {
        avcodec_close(m_video_sw_dec_ctx);
        avcodec_free_context(&m_video_sw_dec_ctx);
    }
    if (m_video_reuse_frame) {
        av_frame_free(&m_video_reuse_frame);
    }

    if (m_audio_hw_dec_ctx) {
        delete m_audio_hw_dec_ctx;
    }
    if (m_audio_hw_dec) {
        js_MediaCodec_stop(m_audio_hw_dec);
        js_MediaCodec_delete(m_audio_hw_dec);
    }
    if (m_audio_sw_dec_ctx) {
        avcodec_close(m_audio_sw_dec_ctx);
        avcodec_free_context(&m_audio_sw_dec_ctx);
    }
    if (m_audio_reuse_frame) {
        av_frame_free(&m_audio_reuse_frame);
    }

    if (m_decoder_type) {
        free(m_decoder_type);
    }
}


JS_RET JSMediaDecoder::create_video_hw_decoder() {
    JS_RET ret;
    void *format = NULL;
    void *video_hw_dec = NULL;
    size_t sps_pps_size = 0;
    size_t convert_size = 0;
    uint8_t *convert_buffer = NULL;
    JSMediaDecoderContext *video_hw_dec_ctx = new JSMediaDecoderContext();

    ret = js_AMediaCodecProfile_getProfileFromAVCodecParameters(video_hw_dec_ctx->mime_type,
                                                                &video_hw_dec_ctx->width,
                                                                &video_hw_dec_ctx->height,
                                                                &video_hw_dec_ctx->profile,
                                                                &video_hw_dec_ctx->level,
                                                                m_video_stream->codecpar);
    if (ret != JS_OK) {
        LOGE("%s parse avCodecContext failed.", __func__);
        goto fail;
    }

    LOGD("%s video_hw_dec_ctx->mime_type=%s,"
         "video_hw_dec_ctx->width=%d,"
         "video_hw_dec_ctx->height=%d,"
         "video_hw_dec_ctx->profile=%d,"
         "video_hw_dec_ctx->level=%d",
         __func__,
         video_hw_dec_ctx->mime_type,
         video_hw_dec_ctx->width,
         video_hw_dec_ctx->height,
         video_hw_dec_ctx->profile,
         video_hw_dec_ctx->level);

    video_hw_dec_ctx->codec_name = js_MediaCodecList_getCodecNameByType(
            video_hw_dec_ctx->mime_type,
            video_hw_dec_ctx->profile,
            0,
            0);

    if (!video_hw_dec_ctx->codec_name) {
        LOGE("%s can't get codec name.", __func__);
        goto fail;
    }

    LOGD("%s video_hw_dec_ctx->codec_name=%s", __func__,
         video_hw_dec_ctx->codec_name);


    video_hw_dec = js_MediaCodec_createCodecByName(video_hw_dec_ctx->codec_name);

    if (!video_hw_dec) {
        LOGE("%s can't create decoder by name", __func__);
        goto fail;
    }

    format = js_MediaFormat_new();
    if (!format) {
        LOGE("%s can't new mediaformat", __func__);
        goto fail;
    }

    js_MediaFormat_setString(format, JS_MEDIAFORMAT_KEY_MIME, video_hw_dec_ctx->mime_type);
    js_MediaFormat_setInt32(format, JS_MEDIAFORMAT_KEY_WIDTH, video_hw_dec_ctx->width);
    js_MediaFormat_setInt32(format, JS_MEDIAFORMAT_KEY_HEIGHT, video_hw_dec_ctx->height);
//    js_MediaFormat_setInt32(format, JS_MEDIAFORMAT_KEY_MAX_INPUT_SIZE, 0);

    sps_pps_size = 0;
    convert_size = (size_t) m_video_stream->codecpar->extradata_size + 20;

    convert_buffer = (uint8_t *) calloc(1, convert_size);
    if (!convert_buffer) {
        LOGE("%s sps_pps_buffer: alloc failed", __func__);
        goto fail;
    }

    if (m_video_stream->codecpar->codec_id == AV_CODEC_ID_H264) {//todo 添加更多解码器支持
        if (JS_ERR ==
            convert_sps_pps(m_video_stream->codecpar->extradata,
                            (size_t) m_video_stream->codecpar->extradata_size,
                            convert_buffer, convert_size,
                            &sps_pps_size, &video_hw_dec_ctx->nal_size)) {
            LOGE("%s convert_sps_pps: failed", __func__);
            goto fail;
        }
    } else {
        //hevc
//        if (0 !=
//            convert_hevc_nal_units(opaque->codecpar->extradata, opaque->codecpar->extradata_size,
//                                   convert_buffer, convert_size,
//                                   &sps_pps_size, &opaque->nal_size)) {
//            LOGE("%s:convert_hevc_nal_units: failed", __func__);
//            goto fail;
//        }
    }
    js_MediaFormat_setBuffer(format, "csd-0", convert_buffer, sps_pps_size);
    for (int i = 0; i < sps_pps_size; i += 4) {
        LOGE("csd-0[%d]: %02x%02x%02x%02x", (int) sps_pps_size, (int) convert_buffer[i + 0],
             (int) convert_buffer[i + 1], (int) convert_buffer[i + 2],
             (int) convert_buffer[i + 3]);
    }

    free(convert_buffer);

    ret = js_MediaCodec_configure(video_hw_dec, format, NULL, NULL, 0);
    if (ret != 0) {
        LOGE("%s js_MediaCodec_configure failed", __func__);
        goto fail;
    }

    js_MediaFormat_delete(format);

    ret = js_MediaCodec_start(video_hw_dec);
    if (ret != 0) {
        LOGE("%s js_MediaCodec_start failed", __func__);
        goto fail;
    }

    video_hw_dec_ctx->format = js_MediaCodec_getOutputFormat(video_hw_dec);
    if (!video_hw_dec_ctx->format) {
        LOGD("%s first js_DMediaCodec_getOutputFormat failed", __func__);
    } else {
        LOGD("%s first js_MediaCodec_getOutputFormat success", __func__);
        ret = video_hw_dec_ctx->update_media_decoder_context();
        if (ret != 0) {
            LOGE("%s update_media_codec_dec_context failed", __func__);
            goto fail;
        }
    }

    m_video_hw_dec = video_hw_dec;
    m_video_hw_dec_ctx = video_hw_dec_ctx;
    m_video_hw_dec_ctx->format = format;
    m_video_reuse_frame = av_frame_alloc();

    LOGD("%s success", __func__);
    return JS_OK;

    fail:
    if (video_hw_dec_ctx) {
        delete video_hw_dec_ctx;
    }

    if (video_hw_dec) {
        js_MediaCodec_delete(video_hw_dec);
    }

    if (format) {
        js_MediaFormat_delete(format);
    }
    if (convert_buffer) {
        free(convert_buffer);
    }

    return JS_ERR_HW_DECODER_UNAVAILABLE;
}


JS_RET JSMediaDecoder::create_audio_hw_decoder() {
//    m_audio_reuse_frame = av_frame_alloc();


    return JS_ERR_HW_DECODER_UNAVAILABLE;
}

JS_RET JSMediaDecoder::create_sw_decoder_by_av_stream(AVStream *av_stream) {

    JS_RET ret;
    AVCodecContext *av_codec_ctx = NULL;
    AVCodec *av_codec = NULL;

    AVCodecParameters *avCodecParas = av_stream->codecpar;
    av_codec = avcodec_find_decoder(avCodecParas->codec_id);

    if (av_codec) {
        LOGD("%s find decoder: %d", __func__, avCodecParas->codec_id);
    } else {
        LOGE("%s failed to find av_codec", __func__);
        goto fail;
    }

    //alloc a codecContext
    av_codec_ctx = avcodec_alloc_context3(av_codec);
    if (!av_codec_ctx) {
        LOGE("%s failed to malloc av_codec_ctx", __func__);
        goto fail;
    }

    //transform
    ret = avcodec_parameters_to_context(av_codec_ctx, avCodecParas);
    if (ret < 0) {
        LOGE("%s copy the codec parameters to context fail!>ret:%d", __func__, ret);
        goto fail;
    }

    ret = avcodec_open2(av_codec_ctx, av_codec, NULL);
    if (ret < 0) {
        LOGE("%s unable to open codec!>ret%d:", __func__, ret);
        goto fail;
    }


    if (avCodecParas->codec_type == AVMEDIA_TYPE_AUDIO) {
        m_audio_stream = av_stream;
        m_audio_reuse_frame = av_frame_alloc();
        m_audio_sw_dec_ctx = av_codec_ctx;
        m_audio_sw_dec = av_codec;
    } else if (avCodecParas->codec_type == AVMEDIA_TYPE_VIDEO) {
        m_video_stream = av_stream;
        m_video_reuse_frame = av_frame_alloc();
        m_video_sw_dec_ctx = av_codec_ctx;
        m_video_sw_dec = av_codec;
    } else {
        goto fail;
    }

    return JS_OK;

    fail:
    if (av_codec_ctx) {
        avcodec_close(av_codec_ctx);
        avcodec_free_context(&av_codec_ctx);
    }
    return JS_ERR_SW_DECODER_UNAVAILABLE;
}


JS_RET JSMediaDecoder::create_hw_decoder_by_av_stream(AVStream *av_stream) {

    JS_RET ret;

    AVMediaType media_type = av_stream->codecpar->codec_type;

    if (media_type == AVMEDIA_TYPE_AUDIO) {
        //todo complete create_audio_hw_decoder func.
//        m_audio_stream = av_stream;
//        ret = create_audio_hw_decoder();
//        if (ret != JS_OK) {
//            LOGE("%s failed to create_audio_hw_decoder", __func__);
//            goto judge_use_sw_decoder;
//        }

        ret = create_sw_decoder_by_av_stream(av_stream);
        if (ret != JS_OK) {
            LOGE("%s failed to create_sw_decoder_by_av_stream", __func__);
            return ret;
        }

    } else if (media_type == AVMEDIA_TYPE_VIDEO) {
        m_video_stream = av_stream;
        ret = create_video_hw_decoder();
        if (ret != JS_OK) {
            LOGE("%s failed to create_video_hw_decoder", __func__);
            goto judge_use_sw_decoder;
        }

    } else {
        LOGE("%s unsupport media_type %d=", __func__,
             media_type);
        return JS_ERR;
    }

    return JS_OK;


    judge_use_sw_decoder:
    if (!strcmp(m_decoder_type, JS_OPTION_DECODER_TYPE_AUTO)) {
        ret = create_sw_decoder_by_av_stream(av_stream);
        if (ret != JS_OK) {
            LOGE("%s failed to create_sw_decoder_by_av_stream", __func__);
            return ret;
        }
    } else {
        return ret;
    }

    update_funcs_by_decode_type(media_type, JS_OPTION_DECODER_TYPE_SW);
    return JS_OK;
}

/**
 * ﻿  if (pkt->size == 0) {
        need_draining = 1;
    }

 */

JS_RET
JSMediaDecoder::process_hw_decode(
        AVPacket *avpkt,
        AVFrame *frame) {

    int index = 0;
    int ret = 0;
    uint8_t *buffer = NULL;
    size_t buffer_size = 0;
    uint32_t flags = 0;
    JSMediaCodecBufferInfo info = {0};

    index = (int) js_MediaCodec_dequeueInputBuffer(m_video_hw_dec,
                                                   INPUT_DEQUEUE_TIMEOUT_US);

    LOGD("%s dequeueInputBuffer index=%d", __func__, index);

    if (index >= 0) {
        buffer = js_MediaCodec_getInputBuffer(m_video_hw_dec, (size_t) index,
                                              &buffer_size);//todo buffer_size> pkt->size?

        if (!buffer) {
            LOGE("%s failed to get input buffer", __func__);
            goto get_output_buffer;
        }
//        LOGD("buffer_size=%d,avpkt->size=%d,buffer_size>avpkt->size=%d", buffer_size, avpkt->size,
//             buffer_size > avpkt->size);

        memcpy(buffer, avpkt->data, (size_t) avpkt->size);

        if (avpkt->flags & AV_PKT_FLAG_KEY) {
            flags |= BUFFER_FLAG_KEY_FRAME;
        }// todo 补充 eos

        LOGD("%s queueInputBuffer flags=%d", __func__, flags);

//        int num = m_video_sw_dec_ctx->time_base.num;
//        int den = m_video_sw_dec_ctx->time_base.den;
//        int rate = m_video_sw_dec_ctx->time_base.num / m_video_sw_dec_ctx->time_base.den;
//        LOGE(" num=%d,den=%d, rate=%d", num,
//             den, rate);//  num=0,den=2, rate=0

//        int num = m_video_sw_dec_ctx->pkt_timebase.num;
//        int den = m_video_sw_dec_ctx->pkt_timebase.den;
//        int rate = m_video_sw_dec_ctx->pkt_timebase.num / m_video_sw_dec_ctx->pkt_timebase.den;
//        LOGE(" num=%d,den=%d, rate=%d", num,
//             den, rate);//  num=0,den=1, rate=0

        int64_t pts = avpkt->pts;

        //AVPacket: presentationTimeUs calculate.  such as m_video_stream->time_base=1/1000  AV_TIME_BASE_Q=1/10000000
        if (m_video_stream->time_base.num && m_video_stream->time_base.den) {
            pts = av_rescale_q(pts, m_video_stream->time_base,
                               AV_TIME_BASE_Q);
        }


        LOGD("%s js_MediaCodec_queueInputBuffer pts=%" PRId64, __func__,
             pts);

        ret = js_MediaCodec_queueInputBuffer(m_video_hw_dec, (size_t) index, 0,
                                             (size_t) avpkt->size,
                                             (uint64_t) pts, flags);
        if (ret < 0) {
            LOGD("%s failed to queue input buffer (status = %d)", __func__, ret);
        }

    } else if (AMEDIACODEC_INFO_TRY_AGAIN_LATER == index) {
        LOGD("%s index input js_MediaCodec_infoTryAgainLater", __func__);
    }

    get_output_buffer:
    index = (int) js_MediaCodec_dequeueOutputBuffer(m_video_hw_dec, &info,
                                                    OUTPUT_DEQUEUE_TIMEOUT_US);

    LOGD("%s dequeueOutputBuffer index=%d,info.size=%d,info.flags=%d,info.presentationTimeUs=%lld,info.offset=%d",
         __func__,
         index, info.size, info.flags, info.presentationTimeUs,
         info.offset);

    if (index >= 0) {
        /**
         *     if (info.flags & MEDIACODEC_BUFFER_FLAG_END_OF_STREM) {
                LOGV("output EOS");
                d->sawOutputEOS = true;
            }
         */

        if (info.size) {
            buffer = js_MediaCodec_getOutputBuffer(m_video_hw_dec, (size_t) index, &buffer_size);

            if (!buffer) {
                LOGE("%s failed to get output buffer", __func__);
                return JS_ERR;
            }

            ret = convert_color_format(m_video_hw_dec_ctx, buffer, buffer_size,
                                       frame);
            js_MediaCodec_releaseOutputBuffer(m_video_hw_dec, (size_t) index, 0);


            if (m_video_stream->time_base.num && m_video_stream->time_base.den) {
                frame->pts = av_rescale_q(info.presentationTimeUs, AV_TIME_BASE_Q,
                                          m_video_stream->time_base);
            } else {
                frame->pts = info.presentationTimeUs;
            }

            LOGD("%s js_MediaCodec_getOutputBuffer pts=%" PRId64, __func__, frame->pts);
            return ret;


        } else {//fixme outputbufferInfo_size==0 is eos mark.
            ret = js_MediaCodec_releaseOutputBuffer(m_video_hw_dec, (size_t) index, 0);
            if (ret < 0) {
                LOGD("%s failed to release output buffer", __func__);
            }
        }

    } else if (AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED == index) {
        LOGD("%s index js_MediaCodec_infoOutputFormatChanged", __func__);

        if (m_video_hw_dec_ctx->format) {
            ret = js_MediaFormat_delete(m_video_hw_dec_ctx->format);
            if (ret < 0) {
                LOGE("%s failed to delete MediaFormat %p", __func__, m_video_hw_dec_ctx->format);
            }
        }

        m_video_hw_dec_ctx->format = js_MediaCodec_getOutputFormat(m_video_hw_dec);
        if (!m_video_hw_dec_ctx->format) {
            LOGE("%s failed to get output format", __func__);
            return JS_ERR_HW_DECODER_UNAVAILABLE;
        }

        ret = m_video_hw_dec_ctx->update_media_decoder_context();
        if (ret != JS_OK) {
            LOGE("%s update_media_codec_dec_context failed", __func__);
            return JS_ERR_HW_DECODER_UNAVAILABLE;
        }
    } else if (AMEDIACODEC_INFO_OUTPUT_BUFFERS_CHANGED == index) {
        LOGD("%s index js_MediaCodec_infoOutputBuffersChanged", __func__);

        if (!__JS_NDK_MEDIACODEC_LINKED__) {
            js_MediaCodec_cleanOutputBuffers((JSMediaCodec *) m_video_hw_dec);
        }
    } else if (AMEDIACODEC_INFO_TRY_AGAIN_LATER == index) {
        LOGD("%s index out put js_MediaCodec_infoTryAgainLater", __func__);

    } else {
        LOGD("%s failed to dequeue output buffer (ret=%zd)", __func__, index);
    }

    return JS_ERR;
}

void JSMediaDecoder::set_decoder_type(const char *decoder_type) {
    m_decoder_type = (char *) malloc(strlen(decoder_type));
    sprintf(m_decoder_type, "%s", decoder_type);
    LOGD("m_decoder_type=%s", m_decoder_type);

    if (!strcmp(decoder_type, JS_OPTION_DECODER_TYPE_HW) ||
        !strcmp(decoder_type, JS_OPTION_DECODER_TYPE_AUTO)) {
        m_create_decoder_by_av_stream = &JSMediaDecoder::create_hw_decoder_by_av_stream;
    } else {
        m_create_decoder_by_av_stream = &JSMediaDecoder::create_sw_decoder_by_av_stream;
    }
    update_funcs_by_decode_type(AVMEDIA_TYPE_AUDIO, m_decoder_type);
    update_funcs_by_decode_type(AVMEDIA_TYPE_VIDEO, m_decoder_type);
}

void JSMediaDecoder::update_funcs_by_decode_type(const AVMediaType media_type,
                                                 const char *decoder_type) {

    if (!strcmp(decoder_type, JS_OPTION_DECODER_TYPE_HW) ||
        !strcmp(decoder_type, JS_OPTION_DECODER_TYPE_AUTO)) {
        if (media_type == AVMEDIA_TYPE_AUDIO) {
            //        m_decode_audio_packet = &JSMediaDecoder::decode_audio_packet_with_hw;
            m_decode_audio_packet = &JSMediaDecoder::decode_audio_packet_with_sw;
//        m_audio_flush = &JSMediaDecoder::audio_hw_decoder_flush;
            m_audio_flush = &JSMediaDecoder::audio_sw_decoder_flush;
        } else if (media_type == AVMEDIA_TYPE_VIDEO) {
            m_decode_video_packet = &JSMediaDecoder::decode_video_packet_with_hw;
            m_video_flush = &JSMediaDecoder::video_hw_decoder_flush;
        }
    } else {
        if (media_type == AVMEDIA_TYPE_AUDIO) {
            m_decode_audio_packet = &JSMediaDecoder::decode_audio_packet_with_sw;
            m_audio_flush = &JSMediaDecoder::audio_sw_decoder_flush;
        } else if (media_type == AVMEDIA_TYPE_VIDEO) {
            m_decode_video_packet = &JSMediaDecoder::decode_video_packet_with_sw;
            m_video_flush = &JSMediaDecoder::video_sw_decoder_flush;
        }
    }
}


AVFrame *JSMediaDecoder::decode_audio_packet_with_hw(AVPacket *avpkt) {
    return NULL;
}

AVFrame *JSMediaDecoder::decode_video_packet_with_hw(AVPacket *avpkt) {

    AVFrame *frame = NULL;
    H264ConvertState convert_state = {0, 0};

    frame = av_frame_alloc();
    if (!frame) {
        LOGE("%s unable to allocate an AVFrame!", __func__);
        return NULL;
    }
    frame->key_frame = (avpkt->flags & AV_PKT_FLAG_KEY) ? 1 : 0;
    LOGD("%s is key_frame :%d", __func__, frame->key_frame);


    convert_h264_to_annexb(avpkt->data, (size_t) avpkt->size, m_video_hw_dec_ctx->nal_size,
                           &convert_state);


    //todo while and flush and drop.
    JS_RET ret = process_hw_decode(avpkt, frame);
    if (ret == JS_OK) {
        return frame;
    } else if (ret == JS_ERR_HW_DECODER_UNAVAILABLE) {
        if (!strcmp(m_decoder_type, JS_OPTION_DECODER_TYPE_AUTO)) {
            ret = create_sw_decoder_by_av_stream(m_video_stream);
            if (ret != JS_OK) {
                LOGE("%s failed to create_sw_decoder_by_av_stream", __func__);
                m_js_event_handler->call_on_error(JS_ERR_SW_DECODER_UNAVAILABLE, 0, 0);
            } else {
                update_funcs_by_decode_type(AVMEDIA_TYPE_VIDEO, JS_OPTION_DECODER_TYPE_SW);
            }
        } else {
            m_js_event_handler->call_on_error(JS_ERR_HW_DECODER_UNAVAILABLE, 0, 0);
        }
    }


    av_frame_free(&frame);
    return frame;
}


AVFrame *JSMediaDecoder::decode_audio_packet_with_sw(AVPacket *avpkt) {
    return decode_packet_with_sw_internal(m_audio_sw_dec_ctx, avpkt, m_audio_reuse_frame);
}

AVFrame *JSMediaDecoder::decode_video_packet_with_sw(AVPacket *avpkt) {
    return decode_packet_with_sw_internal(m_video_sw_dec_ctx, avpkt, m_video_reuse_frame);
}


inline AVFrame *
JSMediaDecoder::decode_packet_with_sw_internal(AVCodecContext *avctx, AVPacket *avpkt,
                                               AVFrame *frame) {

    int ret = avcodec_send_packet(avctx, avpkt);

    if (ret != 0) {
        LOGE("%s avcodec_send_packet failed,result=%d", __func__, ret);
        return NULL;
    }

    ret = avcodec_receive_frame(avctx, frame);

    if (ret != 0) {
        LOGE("%s avcodec_receive_frame failed,result=%d", __func__, ret);
        return NULL;
    }

    AVFrame *tmp = av_frame_alloc();
    if (tmp == NULL) {
        LOGE("%s unable to allocate an AVFrame!", __func__);
        goto done;
    }

    if (av_frame_ref(tmp, frame) != 0) {
        LOGE("%s can't av_frame_ref", __func__);
        av_frame_free(&tmp);
        goto done;
    }

    done:
    av_frame_unref(frame);
    return tmp;
}

void JSMediaDecoder::audio_sw_decoder_flush() {
    m_audio_sw_dec->flush;
}

void JSMediaDecoder::audio_hw_decoder_flush() {
    js_MediaCodec_flush(m_audio_sw_dec);
    avcodec_flush_buffers(m_audio_sw_dec_ctx);
}

void JSMediaDecoder::video_sw_decoder_flush() {
    m_video_sw_dec->flush;//fixme don't know is can work.
}

void JSMediaDecoder::video_hw_decoder_flush() {
    js_MediaCodec_flush(m_video_hw_dec);
    avcodec_flush_buffers(m_video_sw_dec_ctx);
}
