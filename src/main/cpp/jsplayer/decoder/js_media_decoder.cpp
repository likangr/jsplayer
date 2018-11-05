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

#define INPUT_DEQUEUE_TIMEOUT_US 20*1000
#define OUTPUT_DEQUEUE_TIMEOUT_US 20*1000

/**
 * constructor
 */
JSMediaDecoder::JSMediaDecoder() {

}

/**
 * destructor
 */
JSMediaDecoder::~JSMediaDecoder() {
    //video.
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

    //audio.
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

}


JS_RET JSMediaDecoder::create_decoder(AVStream *av_stream) {

    JS_RET ret;

    ret = create_sw_decoder(av_stream);

    if (ret == JS_ERR) {
        LOGE("create_decoder failed to create_sw_decoder");
        return JS_ERR;
    }

    AVMediaType media_type = av_stream->codecpar->codec_type;
    if (media_type == AVMEDIA_TYPE_AUDIO) {
        m_audio_hw_dec_available = false;//todo create audio hw decoder.
        m_audio_stream = av_stream;
    } else if (media_type == AVMEDIA_TYPE_VIDEO) {
        ret = create_hw_decoder(av_stream);
        m_video_hw_dec_available = ret != JS_ERR;
        m_video_stream = av_stream;
    } else {
        LOGE("unsupport media_type except for AVMEDIA_TYPE_AUDIO and AVMEDIA_TYPE_VIDEO .this is %d=",
             media_type);
        return JS_ERR;
    }


    return JS_OK;

}

JS_RET JSMediaDecoder::flush() {

//    if (!m_video_hw_dec_available) {
//        m_video_sw_dec->flush;
//        return JS_OK;
//    }

    int ret;
    ret = js_MediaCodec_flush(m_video_hw_dec);
    if (ret < 0) {
        return JS_ERR;
    }

    return JS_OK;
}


JS_RET JSMediaDecoder::create_hw_decoder(AVStream *av_stream) {
    JS_RET ret;
    void *format = NULL;
    m_video_hw_dec_ctx = new JSMediaDecoderContext;
    size_t sps_pps_size;
    size_t convert_size;
    uint8_t *convert_buffer;


    ret = js_parseAVCodecContext(m_video_hw_dec_ctx->mime_type,
                                 &m_video_hw_dec_ctx->width,
                                 &m_video_hw_dec_ctx->height,
                                 &m_video_hw_dec_ctx->profile,
                                 &m_video_hw_dec_ctx->level,
                                 m_video_sw_dec_ctx);
    if (ret == JS_ERR) {
        LOGE("parse avCodecContext failed.");
        goto fail;
    }
    LOGD("m_video_hw_dec_ctx->mime_type=%s,"
         "m_video_hw_dec_ctx->width=%d,"
         "m_video_hw_dec_ctx->height=%d,"
         "m_video_hw_dec_ctx->profile=%d,"
         "m_video_hw_dec_ctx->level=%d",
         m_video_hw_dec_ctx->mime_type,
         m_video_hw_dec_ctx->width,
         m_video_hw_dec_ctx->height,
         m_video_hw_dec_ctx->profile, m_video_hw_dec_ctx->level);

    m_video_hw_dec_ctx->codec_name = js_MediaCodecList_getCodecNameByType(
            m_video_hw_dec_ctx->mime_type,
            m_video_hw_dec_ctx->profile,
            0,
            0);

    if (!m_video_hw_dec_ctx->codec_name) {
        LOGE("create_decoder can't get codec name.");
        goto fail;
    }

    LOGD("m_video_hw_dec_ctx->codec_name=%s",
         m_video_hw_dec_ctx->codec_name);


    m_video_hw_dec = js_MediaCodec_createCodecByName(m_video_hw_dec_ctx->codec_name);

    if (!m_video_hw_dec) {
        LOGE("create_decoder can't create decoder by name");
        goto fail;
    }

    format = js_MediaFormat_new();
    if (!format) {
        LOGE("create_decoder can't new mediaformat");
        goto fail;
    }

    js_MediaFormat_setString(format, JS_MEDIAFORMAT_KEY_MIME, m_video_hw_dec_ctx->mime_type);
    js_MediaFormat_setInt32(format, JS_MEDIAFORMAT_KEY_WIDTH, m_video_hw_dec_ctx->width);
    js_MediaFormat_setInt32(format, JS_MEDIAFORMAT_KEY_HEIGHT, m_video_hw_dec_ctx->height);
    //js_MediaFormat_setInt32(format, JS_MEDIAFORMAT_KEY_MAX_INPUT_SIZE, 0);

    sps_pps_size = 0;
    convert_size = (size_t) av_stream->codecpar->extradata_size + 20;
    convert_buffer = (uint8_t *) calloc(1, convert_size);
    if (!convert_buffer) {
        LOGE("%s:sps_pps_buffer: alloc failed", __func__);
        goto fail;
    }
    if (av_stream->codecpar->codec_id == AV_CODEC_ID_H264) {//todo 添加更多支持
        if (JS_ERR ==
            convert_sps_pps(av_stream->codecpar->extradata,
                            (size_t) av_stream->codecpar->extradata_size,
                            convert_buffer, convert_size,
                            &sps_pps_size, &m_video_hw_dec_ctx->nal_size)) {
            LOGE("%s:convert_sps_pps: failed", __func__);
            free(convert_buffer);
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


    ret = js_MediaCodec_configure(m_video_hw_dec, format, NULL, NULL, 0);
    if (ret != 0) {
        LOGE("create_decoder js_MediaCodec_configure failed");
        goto fail;
    }

    js_MediaFormat_delete(format);

    ret = js_MediaCodec_start(m_video_hw_dec);
    if (ret != 0) {
        LOGE("create_decoder js_MediaCodec_start failed");
        goto fail;
    }

    m_video_hw_dec_ctx->format = js_MediaCodec_getOutputFormat(m_video_hw_dec);
    if (!m_video_hw_dec_ctx->format) {
        LOGE("create_decoder first js_MediaCodec_getOutputFormat failed");
    } else {
        LOGE("create_decoder first js_MediaCodec_getOutputFormat success");
        ret = m_video_hw_dec_ctx->update_media_decoder_context();
        if (ret != 0) {
            LOGE("create_decoder update_media_codec_dec_context failed");
            goto fail;
        }
    }

    LOGD("create_hw_decoder success");
    return JS_OK;

    fail:
    if (m_video_hw_dec_ctx) {
        delete m_video_hw_dec_ctx;
        m_video_hw_dec_ctx = NULL;
    }
    if (m_video_hw_dec) {
        js_MediaCodec_delete(m_video_hw_dec);
        m_video_hw_dec = NULL;
    }

    if (format) {
        js_MediaFormat_delete(format);
    }

    return JS_ERR;
}


JS_RET JSMediaDecoder::create_sw_decoder(AVStream *av_stream) {

    JS_RET ret;
    AVCodecContext *av_codec_ctx = NULL;
    AVCodec *av_codec = NULL;

    AVCodecParameters *avCodecParas = av_stream->codecpar;
    av_codec = avcodec_find_decoder(avCodecParas->codec_id);

    if (av_codec) {
        LOGD("find decoder: %d", avCodecParas->codec_id);
    } else {
        LOGE("failed to find av_codec");
        goto fail;
    }

    //alloc a codecContext
    av_codec_ctx = avcodec_alloc_context3(av_codec);
    if (!av_codec_ctx) {
        LOGE("failed to malloc av_codec_ctx");
        goto fail;
    }

    //transform
    ret = avcodec_parameters_to_context(av_codec_ctx, avCodecParas);
    if (ret < 0) {
        LOGE("copy the codec parameters to context fail!>ret:%d", ret);
        goto fail;
    }

    ret = avcodec_open2(av_codec_ctx, av_codec, NULL);
    if (ret < 0) {
        LOGE("unable to open codec!>ret%d:", ret);
        goto fail;
    }


    if (avCodecParas->codec_type == AVMEDIA_TYPE_AUDIO) {
        m_audio_sw_dec_ctx = av_codec_ctx;
        m_audio_sw_dec = av_codec;
    } else if (avCodecParas->codec_type == AVMEDIA_TYPE_VIDEO) {
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
    return JS_ERR;
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

    index = js_MediaCodec_dequeueInputBuffer(m_video_hw_dec,
                                             INPUT_DEQUEUE_TIMEOUT_US);
    LOGD("dequeueInputBuffer index=%d",
         index);
    if (AMEDIACODEC_INFO_TRY_AGAIN_LATER == index) {
        goto get_output_buffer;
    }

    if (index >= 0) {

        buffer = js_MediaCodec_getInputBuffer(m_video_hw_dec, (size_t) index,
                                              &buffer_size);//todo buffer_size> pkt->size?

        if (!buffer) {
            LOGE("failed to get input buffer");
            goto get_output_buffer;
        }

        memcpy(buffer, avpkt->data, (size_t) avpkt->size);

        if (avpkt->flags & AV_PKT_FLAG_KEY) {
            flags |= BUFFER_FLAG_KEY_FRAME;
        }// 补充 eos

        LOGD("queueInputBuffer flags=%d", flags);

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
        if (pts == AV_NOPTS_VALUE && avpkt->dts != AV_NOPTS_VALUE)
            pts = avpkt->dts;
        if (pts >= 0) {
            pts = av_rescale_q(pts, m_video_stream->time_base, AV_TIME_BASE_Q);
        } else {
            pts = 0;
        }
        LOGD("js_MediaCodec_queueInputBuffer pts=%" PRId64, pts);

        ret = js_MediaCodec_queueInputBuffer(m_video_hw_dec, (size_t) index, 0,
                                             (size_t) avpkt->size,
                                             (uint64_t) pts, flags);
        if (ret < 0) {
            LOGD("failed to queue input buffer (status = %d)", ret);
        }
    }


    get_output_buffer:

    index = js_MediaCodec_dequeueOutputBuffer(m_video_hw_dec, &info,
                                              OUTPUT_DEQUEUE_TIMEOUT_US);

    LOGD("dequeueOutputBuffer index=%d,info.size=%d,info.flags=%d,info.presentationTimeUs=%lld,info.offset=%d",
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
                LOGE("Failed to get output buffer");
                return JS_ERR;
            }

            ret = convert_color_format(m_video_hw_dec_ctx, buffer, buffer_size,
                                       frame);
            js_MediaCodec_releaseOutputBuffer(m_video_hw_dec, (size_t) index, 0);
            frame->pts = av_rescale_q(info.presentationTimeUs, AV_TIME_BASE_Q,
                                      m_video_stream->time_base);
            LOGD("js_MediaCodec_getOutputBuffer pts=%" PRId64, frame->pts);
            return ret;


        } else {//fixme outputbufferInfo_size==0 is eos mark.
            ret = js_MediaCodec_releaseOutputBuffer(m_video_hw_dec, (size_t) index, 0);
            if (ret < 0) {
                LOGD("failed to release output buffer");
            }
        }

    } else if (AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED == index) {
        LOGD("index js_MediaCodec_infoOutputFormatChanged");

        if (m_video_hw_dec_ctx->format) {
            ret = js_MediaFormat_delete(m_video_hw_dec_ctx->format);
            if (ret < 0) {
                LOGE("Failed to delete MediaFormat %p", m_video_hw_dec_ctx->format);
            }
        }

        m_video_hw_dec_ctx->format = js_MediaCodec_getOutputFormat(m_video_hw_dec);
        if (!m_video_hw_dec_ctx->format) {
            LOGE("failed to get output format");
            return JS_ERR_MEDIACODEC_UNAVAILABLE;
        }

        ret = m_video_hw_dec_ctx->update_media_decoder_context();
        if (ret != JS_OK) {
            LOGE("update_media_codec_dec_context failed");
            return JS_ERR_MEDIACODEC_UNAVAILABLE;
        }
    } else if (AMEDIACODEC_INFO_OUTPUT_BUFFERS_CHANGED == index) {
        LOGD("index js_MediaCodec_infoOutputBuffersChanged");

        if (!__JS_NDK_MEDIACODEC_LINKED__) {
            js_MediaCodec_cleanOutputBuffers((JSMediaCodec *) m_video_hw_dec);
        }
    } else if (AMEDIACODEC_INFO_TRY_AGAIN_LATER == index) {
        LOGD("index js_MediaCodec_infoTryAgainLater");

    } else {
        LOGD("Failed to dequeue output buffer (ret=%zd)", index);
    }

    return JS_ERR;
}

void JSMediaDecoder::set_decode_packet_type(const char *decode_type) {
    if (!strcmp(decode_type, JS_OPTION_DECODER_TYPE_HW)) {
        m_decode_video_packet = &JSMediaDecoder::decode_video_packet_with_hw;
//        m_decode_audio_packet = decode_audio_packet_with_hw;
        m_decode_audio_packet = &JSMediaDecoder::decode_audio_packet_with_sw;
    } else {
        m_decode_audio_packet = &JSMediaDecoder::decode_audio_packet_with_sw;
        m_decode_video_packet = &JSMediaDecoder::decode_video_packet_with_sw;
    }
}


//﻿    AVRational             frame_rate    = av_guess_frame_rate(is->ic, is->video_st, NULL);
AVFrame *JSMediaDecoder::decode_audio_packet_with_hw(AVPacket *avpkt) {
    return NULL;
}

//﻿    AVRational             frame_rate    = av_guess_frame_rate(is->ic, is->video_st, NULL);
AVFrame *JSMediaDecoder::decode_video_packet_with_hw(AVPacket *avpkt) {

    AVFrame *frame = NULL;
    H264ConvertState convert_state = {0, 0};
    if (!m_video_hw_dec_available) {
        return decode_video_packet_with_sw(avpkt);
    }

    frame = av_frame_alloc();
    if (!frame) {
        LOGE("unable to allocate an AVFrame!");
        return NULL;
    }
    frame->key_frame = (avpkt->flags & AV_PKT_FLAG_KEY) ? 1 : 0;
    LOGD("is key_frame :%d", frame->key_frame);


    convert_h264_to_annexb(avpkt->data, (size_t) avpkt->size, m_video_hw_dec_ctx->nal_size,
                           &convert_state);

    JS_RET ret = process_hw_decode(avpkt, frame);

    if (ret == JS_OK) {
        return frame;
    }

    if (ret == JS_ERR_MEDIACODEC_UNAVAILABLE) {
        m_video_hw_dec_available = false;
    }

    av_frame_free(&frame);//fixme 关键帧 解码 失败， 整个 gop 都应该丢弃。
    return frame;
}


AVFrame *JSMediaDecoder::decode_audio_packet_with_sw(AVPacket *avpkt) {

    int ret = avcodec_send_packet(m_audio_sw_dec_ctx, avpkt);
    if (ret != 0) {
        LOGE("avcodec_send_packet failed,result=%d", ret);
        return NULL;
    }

    AVFrame *tmp = av_frame_alloc();
    if (tmp == NULL) {
        LOGE("Unable to allocate an AVFrame!");
        return NULL;
    }
    ret = avcodec_receive_frame(m_audio_sw_dec_ctx, tmp);

    if (ret != 0) {
        LOGE("avcodec_receive_frame failed,result=%d", ret);
        av_frame_free(&tmp);
        return NULL;
    }
    return tmp;
}

AVFrame *JSMediaDecoder::decode_video_packet_with_sw(AVPacket *avpkt) {

    int ret = avcodec_send_packet(m_video_sw_dec_ctx, avpkt);

    if (ret != 0) {
        LOGE("avcodec_send_packet failed,result=%d", ret);
        return NULL;
    }

    AVFrame *tmp = av_frame_alloc();
    if (tmp == NULL) {
        LOGE("Unable to allocate an AVFrame!");
        return NULL;
    }
    ret = avcodec_receive_frame(m_video_sw_dec_ctx, tmp);

    if (ret != 0) {
        LOGE("avcodec_receive_frame failed,result=%d", ret);
        av_frame_free(&tmp);
        return NULL;
    }
    return tmp;
}

