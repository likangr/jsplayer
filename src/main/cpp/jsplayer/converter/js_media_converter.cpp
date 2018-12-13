#include "js_media_converter.h"
#include "libyuv/convert.h"
#include "libyuv.h"


JS_RET JSMediaCoverter::init_audio_converter(struct SwrContext *s,
                                             int64_t out_ch_layout,
                                             int out_channel,
                                             enum AVSampleFormat out_sample_fmt,
                                             int out_sample_rate,
                                             int bytes_per_sample,
                                             int64_t in_ch_layout,
                                             int in_channel,
                                             enum AVSampleFormat in_sample_fmt,
                                             int in_sample_rate) {
    m_out_ch_layout = out_ch_layout;
    m_out_channel = out_channel;
    m_out_sample_fmt = out_sample_fmt;
    m_out_sample_rate = out_sample_rate;
    m_bytes_per_sample = bytes_per_sample;
    m_in_ch_layout = in_ch_layout;
    m_in_channel = in_channel;
    m_in_sample_fmt = in_sample_fmt;
    m_in_sample_rate = in_sample_rate;

    LOGD("%s m_out_ch_layout=%lld,"
         "m_out_channel=%d,"
         "m_out_sample_fmt=%d,"
         "m_out_sample_rate=%d,"
         "m_bytes_per_sample=%d,"
         "m_in_ch_layout=%lld,"
         "m_in_channel=%d,"
         "m_in_sample_fmt=%d,"
         "m_in_sample_rate=%d",
         __func__,
         m_out_ch_layout,
         m_out_channel,
         m_out_sample_fmt,
         m_out_sample_rate,
         m_bytes_per_sample,
         m_in_ch_layout,
         m_in_channel,
         m_in_sample_fmt,
         m_in_sample_rate);

    m_audio_swr_ctx = swr_alloc_set_opts(s, m_out_ch_layout,
                                         m_out_sample_fmt,
                                         m_out_sample_rate,
                                         m_in_ch_layout,
                                         m_in_sample_fmt,
                                         m_in_sample_rate, 0, NULL);
    if (!m_audio_swr_ctx || swr_init(m_audio_swr_ctx) < 0) {
        return JS_ERR_EXTERNAL;
    }

    return JS_OK;
}

void JSMediaCoverter::release_audio_converter() {
    if (m_audio_swr_ctx != NULL) {
        swr_free(&m_audio_swr_ctx);
        m_audio_swr_ctx = NULL;
    }

    m_out_ch_layout = 0;
    m_in_channel = 0;
    m_out_sample_fmt = AV_SAMPLE_FMT_NONE;
    m_out_sample_rate = 0;
    m_bytes_per_sample = 0;
    m_in_ch_layout = 0;
    m_out_channel = 0;
    m_in_sample_fmt = AV_SAMPLE_FMT_NONE;
    m_in_sample_rate = 0;
}


unsigned int
JSMediaCoverter::convert_simple_format_to_S16(uint8_t *convert_audio_buf, AVFrame *frame) {
    //计算目标样本数  转换前后的样本数不一样  抓住一点 采样时间相等
    //src_nb_samples/src_rate=dst_nb_samples/dst_rate

    /**
     * swr_get_delay==》返回之前没有处理的输入采样个数。
     * 如果提供的输入大于可用的输出空间，Swresample可以缓冲数据，《而且在采样率之间的转换需要延迟。不理解这句话？》
     * 这个函数返回所有这些延迟的总和。
     * 准确的延迟不一定是输入或输出采样率的整数值。特别是当下采样值很大时，输出采样率可能不是表示延迟的好选择，就像上采样率和输入采样率一样。
     */
    int out_nb_samples = (int) av_rescale_rnd(
            swr_get_delay(m_audio_swr_ctx, m_in_sample_rate) + frame->nb_samples,
            m_out_sample_rate,
            m_in_sample_rate, AV_ROUND_INF);

    int ret = swr_convert(m_audio_swr_ctx, &convert_audio_buf, out_nb_samples,
                          (const uint8_t **) frame->data,
                          frame->nb_samples);

    if (ret < 0) {
        return 0;
    }

    int data_size = m_out_channel * out_nb_samples * m_bytes_per_sample;

//    LOGD("%s out_nb_samples=%d,data_size=%d", __func__,
//         out_nb_samples, data_size);

    return (unsigned int) data_size;
}


JS_RET fill_video_frame(JSMediaDecoderContext *ctx,
                        JSMediaCodec *video_hw_dec,
                        AVStream *av_stream,
                        JSMediaCodecBufferInfo *info,
                        AVFrame *frame,
                        uint8_t *data,
                        size_t size) {

    int ret;

    av_frame_ref(frame, ctx->frame_buf);

    //fixme maybe inaccuracy.
    frame->key_frame = (info->flags &
                        js_MediaCodec_getBufferFlagKeyFrame(video_hw_dec)) ? 1
                                                                           : 0;
    LOGD("%s frame->key_frame=%d", __func__, frame->key_frame);

    if (av_stream->time_base.num && av_stream->time_base.den) {
        frame->pts = av_rescale_q(info->presentationTimeUs, AV_TIME_BASE_Q,
                                  av_stream->time_base);
    } else {
        frame->pts = info->presentationTimeUs;
    }

    LOGD("%s js_MediaCodec_getOutputBuffer pts=%" PRId64, __func__, frame->pts);


    ret = ConvertToI420(data, size,
                        frame->data[0], frame->linesize[0],
                        frame->data[1], frame->linesize[1],
                        frame->data[2], frame->linesize[2],
                        0, 0,
                        frame->width, frame->height,
                        frame->width, frame->height,
                        libyuv::kRotate0, ctx->yuv_fourcc);

    if (ret != 0) {
        if (ret == -1) {
            LOGE("ConvertToI420 failed because invalid parameter");
        } else {
            LOGE("ConvertToI420 failed don't know why.");
        }

        return JS_ERR;
    }

    return JS_OK;
}


void convert_h264_to_annexb(uint8_t *p_buf, size_t i_len,
                            size_t i_nal_size,
                            H264ConvertState *state) {
    if (i_nal_size < 3 || i_nal_size > 4)
        return;

    /* This only works for NAL sizes 3-4 */
    while (i_len > 0) {
        if (state->nal_pos < i_nal_size) {
            unsigned int i;
            for (i = 0; state->nal_pos < i_nal_size && i < i_len; i++, state->nal_pos++) {
                state->nal_len = (state->nal_len << 8) | p_buf[i];
                p_buf[i] = 0;
            }
            if (state->nal_pos < i_nal_size)
                return;
            p_buf[i - 1] = 1;
            p_buf += i;
            i_len -= i;
        }
        if (state->nal_len > INT_MAX)
            return;
        if (state->nal_len > i_len) {
            state->nal_len -= i_len;
            return;
        } else {
            p_buf += state->nal_len;
            i_len -= state->nal_len;
            state->nal_len = 0;
            state->nal_pos = 0;
        }
    }
}

/* Parse the SPS/PPS Metadata and convert it to annex b format */
int convert_sps_pps(const uint8_t *p_buf, size_t i_buf_size,
                    uint8_t *p_out_buf, size_t i_out_buf_size,
                    size_t *p_sps_pps_size, size_t *p_nal_size) {
    // int i_profile;
    uint32_t i_data_size = i_buf_size, i_nal_size, i_sps_pps_size = 0;
    unsigned int i_loop_end;

    /* */
    if (i_data_size < 7) {

        LOGE("Input Metadata too small");
        return JS_ERR;
    }

    /* Read infos in first 6 bytes */
    // i_profile    = (p_buf[1] << 16) | (p_buf[2] << 8) | p_buf[3];
    if (p_nal_size)
        *p_nal_size = (size_t) (p_buf[4] & 0x03) + 1;
    p_buf += 5;
    i_data_size -= 5;

    for (unsigned int j = 0; j < 2; j++) {
        /* First time is SPS, Second is PPS */
        if (i_data_size < 1) {

            LOGE("PPS too small after processing SPS/PPS %u",
                 i_data_size);
            return JS_ERR;
        }
        i_loop_end = (size_t) p_buf[0] & (j == 0 ? 0x1f : 0xff);
        p_buf++;
        i_data_size--;

        for (unsigned int i = 0; i < i_loop_end; i++) {
            if (i_data_size < 2) {

                LOGE("SPS is too small %u", i_data_size);
                return JS_ERR;
            }

            i_nal_size = (p_buf[0] << 8) | p_buf[1];
            p_buf += 2;
            i_data_size -= 2;

            if (i_data_size < i_nal_size) {

                LOGE("SPS size does not match NAL specified size %u",
                     i_data_size);
                return JS_ERR;
            }
            if (i_sps_pps_size + 4 + i_nal_size > i_out_buf_size) {

                LOGE("Output SPS/PPS buffer too small");
                return JS_ERR;
            }

            p_out_buf[i_sps_pps_size++] = 0;
            p_out_buf[i_sps_pps_size++] = 0;
            p_out_buf[i_sps_pps_size++] = 0;
            p_out_buf[i_sps_pps_size++] = 1;

            memcpy(p_out_buf + i_sps_pps_size, p_buf, i_nal_size);
            i_sps_pps_size += i_nal_size;

            p_buf += i_nal_size;
            i_data_size -= i_nal_size;
        }
    }

    *p_sps_pps_size = i_sps_pps_size;

    return JS_OK;
}