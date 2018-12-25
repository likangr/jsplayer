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

#define INPUT_DEQUEUE_TIMEOUT_US            8000
#define OUTPUT_DEQUEUE_TIMEOUT_US           8000

JSMediaDecoder::JSMediaDecoder(JSEventHandler *js_eventHandler) {
    m_js_event_handler = js_eventHandler;
}

JSMediaDecoder::~JSMediaDecoder() {

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
            false,
            NULL);

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
        LOGD("%s csd-0[%d]: %02x%02x%02x%02x", __func__, (int) sps_pps_size,
             (int) convert_buffer[i + 0],
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
        LOGE("%s copy the codec parameters to context fail! ret:%d", __func__, ret);
        goto fail;
    }

    ret = avcodec_open2(av_codec_ctx, av_codec, NULL);
    if (ret < 0) {
        LOGE("%s unable to open codec!>ret%d:", __func__, ret);
        goto fail;
    }


    if (avCodecParas->codec_type == AVMEDIA_TYPE_AUDIO) {
        m_audio_stream = av_stream;
        m_audio_sw_dec_ctx = av_codec_ctx;
        m_audio_sw_dec = av_codec;
    } else if (avCodecParas->codec_type == AVMEDIA_TYPE_VIDEO) {
        m_video_stream = av_stream;
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


JS_RET JSMediaDecoder::mediacodecdec_receive_frame(AVFrame *frame) {

    int index = 0;
    int ret = 0;
    uint8_t *buffer = NULL;
    size_t buffer_size = 0;
    JSMediaCodecBufferInfo info = {0};

    index = (int) js_MediaCodec_dequeueOutputBuffer(m_video_hw_dec, &info,
                                                    OUTPUT_DEQUEUE_TIMEOUT_US);

    LOGD("%s dequeueOutputBuffer index=%d,info.size=%d,info.flags=%d,info.presentationTimeUs=%lld,info.offset=%d",
         __func__,
         index, info.size, info.flags, info.presentationTimeUs,
         info.offset);

    if (index >= 0) {

        if (info.size) {

            buffer = js_MediaCodec_getOutputBuffer(m_video_hw_dec, (size_t) index, &buffer_size);

            if (!buffer) {
                LOGE("%s failed to get output buffer", __func__);
                return JS_ERR_EXTERNAL;
            }

            ret = fill_video_frame(m_video_hw_dec_ctx,
                                   (JSMediaCodec *) (m_video_hw_dec),
                                   m_video_stream,
                                   &info,
                                   frame,
                                   buffer,
                                   buffer_size);

            if (js_MediaCodec_releaseOutputBuffer(m_video_hw_dec, (size_t) index, 0) < 0) {
                LOGD("%s failed to release output buffer", __func__);
            }

            if (ret != JS_OK) {
                LOGD("%s failed to fill video frame", __func__);
                return JS_ERR_EXTERNAL;
            }

        } else {
            if (js_MediaCodec_releaseOutputBuffer(m_video_hw_dec, (size_t) index, 0) < 0) {
                LOGD("%s failed to release output buffer", __func__);
            }

            return JS_ERR_EOF;
        }

    } else if (js_MediaCodec_getInfoOutputFormatChanged((JSMediaCodec *) m_video_hw_dec) == index) {
        LOGD("%s js_MediaCodec_infoOutputFormatChanged", __func__);

        if (m_video_hw_dec_ctx->format) {
            ret = js_MediaFormat_delete(m_video_hw_dec_ctx->format);
            if (ret < 0) {
                LOGE("%s failed to delete MediaFormat %p", __func__, m_video_hw_dec_ctx->format);
            }
        }

        m_video_hw_dec_ctx->format = js_MediaCodec_getOutputFormat(m_video_hw_dec);
        if (!m_video_hw_dec_ctx->format) {
            LOGE("%s failed to get output format", __func__);
            return JS_ERR_EXTERNAL;
        }

        ret = m_video_hw_dec_ctx->update_media_decoder_context();
        if (ret != JS_OK) {
            LOGE("%s update_media_codec_dec_context failed", __func__);
            return JS_ERR_EXTERNAL;
        }

        return JS_ERR_TRY_AGAIN;

    } else if (js_MediaCodec_getInfoOutputBuffersChanged((JSMediaCodec *) m_video_hw_dec) ==
               index) {
        LOGD("%s js_MediaCodec_infoOutputBuffersChanged", __func__);

        js_MediaCodec_cleanOutputBuffers((JSMediaCodec *) m_video_hw_dec);

        return JS_ERR_TRY_AGAIN;
    } else if (js_MediaCodec_getInfoTryAgainLater((JSMediaCodec *) m_video_hw_dec) == index) {
        LOGD("%s js_MediaCodec_getInfoTryAgainLater", __func__);

        return JS_ERR_TRY_AGAIN;
    } else {
        LOGD("%s js_MediaCodec_dequeueOutputBuffer (index=%d)", __func__, index);

        return JS_ERR_EXTERNAL;
    }

    return JS_OK;
}

JS_RET JSMediaDecoder::mediacodecdec_send_packet(AVPacket *avpkt) {

    int index = 0;
    int ret = 0;
    uint8_t *buffer = NULL;
    size_t buffer_size = 0;
    int flags = 0;
    int need_draining = avpkt->size == 0;

    index = (int) js_MediaCodec_dequeueInputBuffer(m_video_hw_dec, INPUT_DEQUEUE_TIMEOUT_US);

    LOGD("%s dequeueInputBuffer index=%d", __func__, index);

    if (index >= 0) {
        buffer = js_MediaCodec_getInputBuffer(m_video_hw_dec, (size_t) index,
                                              &buffer_size);//fixme buffer_size must > pkt->size?

        if (!buffer) {
            LOGE("%s failed to get input buffer", __func__);
            return JS_ERR_EXTERNAL;
        }

        LOGD("%s buffer_size=%d,avpkt->size=%d,buffer_size>avpkt->size=%d", __func__, buffer_size,
             avpkt->size,
             buffer_size > avpkt->size);

        int64_t pts;
        if (need_draining) {

            flags |= js_MediaCodec_getBufferFlagEndOfStream(
                    (JSMediaCodec *) m_video_hw_dec);

//            if (s->surface) {
//                pts = av_rescale_q(pts, avctx->pkt_timebase, av_make_q(1, 1000000));
//            }
            pts = avpkt->pts;

            LOGD("%s send End Of Stream signal", __func__);

        } else {

            /**
            int bufferFlagKeyFrame = MediaCodec.BUFFER_FLAG_KEY_FRAME; api21 fixme
             */
            if (avpkt->flags & AV_PKT_FLAG_KEY) {
                flags |= js_MediaCodec_getBufferFlagKeyFrame((JSMediaCodec *) m_video_hw_dec);
            }

            pts = avpkt->pts;

            //AVPacket: presentationTimeUs calculate.  such as m_video_stream->time_base=1/1000  AV_TIME_BASE_Q=1/10000000
            if (m_video_stream->time_base.num && m_video_stream->time_base.den) {
                pts = av_rescale_q(pts, m_video_stream->time_base,
                                   AV_TIME_BASE_Q);
            }


            //fixme more codec type.
            H264ConvertState convert_state = {0};
            convert_h264_to_annexb(avpkt->data, (size_t) avpkt->size, m_video_hw_dec_ctx->nal_size,
                                   &convert_state);

            memcpy(buffer, avpkt->data, (size_t) avpkt->size);
        }

        LOGD("%s js_MediaCodec_queueInputBuffer pts=%"
                     PRId64, __func__,
             pts);

        ret = js_MediaCodec_queueInputBuffer(m_video_hw_dec, (size_t) index, 0,
                                             (size_t) avpkt->size,
                                             (uint64_t) pts, flags);

        if (ret < 0) {
            LOGD("%s failed to queue input buffer (status = %d)", __func__, ret);
            return JS_ERR_EXTERNAL;
        }
    } else if (js_MediaCodec_getInfoTryAgainLater((JSMediaCodec *) m_video_hw_dec) == index) {
        LOGD("%s js_MediaCodec_infoTryAgainLater", __func__);
        return JS_ERR_TRY_AGAIN;
    } else {
        LOGD("%s failed to js_MediaCodec_dequeueInputBuffer (index=%d)", __func__, index);
        return JS_ERR_EXTERNAL;
    }

    return JS_OK;
}


void JSMediaDecoder::set_decoder_type(const char *decoder_type) {
    m_decoder_type = (char *) malloc(strlen(decoder_type));
    sprintf(m_decoder_type, "%s", decoder_type);
    LOGD("%s m_decoder_type=%s", __func__,m_decoder_type);

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
            //        decode_audio_packet = &JSMediaDecoder::decode_audio_packet_with_hw;
            decode_audio_packet = &JSMediaDecoder::decode_audio_packet_with_sw;
//        audio_decoder_flush = &JSMediaDecoder::audio_hw_decoder_flush;
            audio_decoder_flush = &JSMediaDecoder::audio_sw_decoder_flush;
        } else if (media_type == AVMEDIA_TYPE_VIDEO) {
            decode_video_packet = &JSMediaDecoder::decode_video_packet_with_hw;
            video_decoder_flush = &JSMediaDecoder::video_hw_decoder_flush;
        }
    } else {
        if (media_type == AVMEDIA_TYPE_AUDIO) {
            decode_audio_packet = &JSMediaDecoder::decode_audio_packet_with_sw;
            audio_decoder_flush = &JSMediaDecoder::audio_sw_decoder_flush;
        } else if (media_type == AVMEDIA_TYPE_VIDEO) {
            decode_video_packet = &JSMediaDecoder::decode_video_packet_with_sw;
            video_decoder_flush = &JSMediaDecoder::video_sw_decoder_flush;
        }
    }
}


JS_RET JSMediaDecoder::decode_audio_packet_with_hw(AVPacket *avpkt,
                                                   AVFrame *frame) {
    return JS_ERR;
}


JS_RET JSMediaDecoder::decode_video_packet_with_hw(AVPacket *avpkt,
                                                   AVFrame *frame) {

    JS_RET ret;
    bool is_send_packet = false;


    while (true) {
        while (true) {

            ret = mediacodecdec_receive_frame(frame);

            if (ret == JS_OK) {
                return is_send_packet ? JS_OK : JS_OK_NOT_DECODE_PACKET;
            } else if (ret == JS_ERR_EOF) {
                return JS_ERR_EOF;
            } else if (ret == JS_ERR_TRY_AGAIN) {
                if (is_send_packet) {
                    return JS_ERR_NEED_SEND_NEW_PACKET_AGAIN;
                } else {
                    break;
                }
            } else {
                LOGW("%s mediacodecdec_receive_frame failed,result=%d", __func__, ret);
                goto hw_decoder_unavailable;
            }
        }


        ret = mediacodecdec_send_packet(avpkt);
        if (ret == JS_OK) {
            is_send_packet = true;
        } else if (ret == JS_ERR_TRY_AGAIN) {
            return JS_ERR_NEED_SEND_THIS_PACKET_AGAIN;
        } else {
            LOGW("%s mediacodecdec_send_packet failed,result=%d", __func__, ret);
            goto hw_decoder_unavailable;
        }

    }

    hw_decoder_unavailable:
    LOGE("%s goto hw decoder unavailable", __func__);
    if (!strcmp(m_decoder_type, JS_OPTION_DECODER_TYPE_AUTO)) {
        ret = create_sw_decoder_by_av_stream(m_video_stream);
        if (ret != JS_OK) {
            LOGE("%s failed to create_sw_decoder_by_av_stream", __func__);
            return JS_ERR_SW_DECODER_UNAVAILABLE;
        } else {
            update_funcs_by_decode_type(AVMEDIA_TYPE_VIDEO, JS_OPTION_DECODER_TYPE_SW);
            return JS_ERR_TRY_TO_USE_SW_DECODER;
        }
    } else {
        return JS_ERR_HW_DECODER_UNAVAILABLE;
    }
}

JS_RET JSMediaDecoder::decode_audio_packet_with_sw(AVPacket *avpkt,
                                                   AVFrame *frame) {
    return decode_packet_with_sw_internal(m_audio_sw_dec_ctx, avpkt, frame);

}

JS_RET JSMediaDecoder::decode_video_packet_with_sw(AVPacket *avpkt,
                                                   AVFrame *frame) {
    return decode_packet_with_sw_internal(m_video_sw_dec_ctx, avpkt, frame);

}


JS_RET
JSMediaDecoder::decode_packet_with_sw_internal(AVCodecContext *avctx,
                                               AVPacket *avpkt,
                                               AVFrame *frame) {

    int ret;
    bool is_send_packet = false;

    while (true) {
        while (true) {

            ret = avcodec_receive_frame(avctx, frame);

            if (ret == 0) {
                return is_send_packet ? JS_OK : JS_OK_NOT_DECODE_PACKET;
            } else if (ret == AVERROR_EOF) {
                return JS_ERR_EOF;
            } else if (ret == AVERROR(EAGAIN)) {
                if (is_send_packet) {
                    return JS_ERR_NEED_SEND_NEW_PACKET_AGAIN;
                } else {
                    break;
                }
            } else {
                return JS_ERR_SW_DECODER_UNAVAILABLE;
            }
        }

        ret = avcodec_send_packet(avctx, avpkt);
        if (ret != 0) {
            LOGE("%s avcodec_send_packet failed,result=%d", __func__, ret);
            return JS_ERR_SW_DECODER_UNAVAILABLE;
        }
        is_send_packet = true;
    }
}

void JSMediaDecoder::audio_sw_decoder_flush() {
    avcodec_flush_buffers(m_audio_sw_dec_ctx);
}

void JSMediaDecoder::audio_hw_decoder_flush() {
    avcodec_flush_buffers(m_audio_sw_dec_ctx);
//    js_MediaCodec_flush(m_audio_hw_dec);
}

void JSMediaDecoder::video_sw_decoder_flush() {
    avcodec_flush_buffers(m_video_sw_dec_ctx);
}

void JSMediaDecoder::video_hw_decoder_flush() {
    js_MediaCodec_flush(m_video_hw_dec);
}

void JSMediaDecoder::reset() {
    if (m_video_hw_dec_ctx) {
        delete m_video_hw_dec_ctx;
        m_video_hw_dec_ctx = NULL;
    }
    if (m_video_hw_dec) {
        js_MediaCodec_stop(m_video_hw_dec);
        js_MediaCodec_delete(m_video_hw_dec);
        m_video_hw_dec = NULL;
    }
    if (m_video_sw_dec_ctx) {
        avcodec_close(m_video_sw_dec_ctx);
        avcodec_free_context(&m_video_sw_dec_ctx);
    }

    if (m_audio_hw_dec_ctx) {
        delete m_audio_hw_dec_ctx;
        m_audio_hw_dec_ctx = NULL;
    }

    if (m_audio_hw_dec) {
        js_MediaCodec_stop(m_audio_hw_dec);
        js_MediaCodec_delete(m_audio_hw_dec);
        m_audio_hw_dec = NULL;
    }

    if (m_audio_sw_dec_ctx) {
        avcodec_close(m_audio_sw_dec_ctx);
        avcodec_free_context(&m_audio_sw_dec_ctx);
    }

    if (m_decoder_type) {
        free(m_decoder_type);
        m_decoder_type = NULL;
    }
}