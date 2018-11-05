#include "js_media_converter.h"
#include "libyuv/convert.h"
#include "libyuv.h"

extern "C" {
#include "util/js_log.h"
#include "libavutil/imgutils.h"
#include "libswresample/swresample.h"
}


#define RENDER_AV_FRAME_FORMAT AV_PIX_FMT_YUV420P

unsigned int convert_simple_format_to_S16(uint8_t *audio_buf, AVFrame *frame) {

    SwrContext *swr_ctx;

    swr_ctx = swr_alloc_set_opts(NULL, frame->channel_layout, DST_FORMAT, frame->sample_rate,
                                 frame->channel_layout,
                                 (enum AVSampleFormat) frame->format,
                                 frame->sample_rate, 0, NULL);
    if (!swr_ctx || swr_init(swr_ctx) < 0) {
        return 0;
    }

    // 计算转换后的sample个数 a * b / c
    int dst_nb_samples = (int) av_rescale_rnd(
            swr_get_delay(swr_ctx, frame->sample_rate) + frame->sample_rate, frame->sample_rate,
            frame->sample_rate, AV_ROUND_INF);


    int nb = swr_convert(swr_ctx, &audio_buf, dst_nb_samples, (const uint8_t **) frame->data,
                         frame->nb_samples);

    if (nb < 0) {
        swr_free(&swr_ctx);
        return 0;
    }

    int out_channels = av_get_channel_layout_nb_channels(frame->channel_layout);
    int data_size = out_channels * nb * av_get_bytes_per_sample(DST_FORMAT);

    swr_free(&swr_ctx);
    return (unsigned int) data_size;
}


JS_RET convert_color_format(
        JSMediaDecoderContext *ctx,
        uint8_t *data,
        size_t size,
        AVFrame *frame) {

    int ret;
    int numBytes = 0;
    uint8_t *buffer = NULL;

    frame->width = ctx->width;
    frame->height = ctx->height;
    frame->format = RENDER_AV_FRAME_FORMAT;


    //malloc frame reuse buffer
    numBytes = av_image_get_buffer_size(RENDER_AV_FRAME_FORMAT, ctx->width, ctx->height, 1);
    if (numBytes <= 0) {
        LOGE("av_image_get_buffer_size failed numBytes:%d", numBytes);
        goto fail;
    }

    buffer = (uint8_t *) av_malloc((size_t) numBytes);
    if (!buffer) {
        LOGE("malloc buffer failed");
        goto fail;
    }

    frame->opaque = buffer;
    ret = av_image_fill_arrays(frame->data, frame->linesize, buffer,
                               (AVPixelFormat) frame->format, ctx->width, ctx->height, 1);
    if (ret < 0) {
        LOGE("convert_color_format av_image_fill_arrays failed");
        goto fail;
    }


    ret = ConvertToI420(data, size,
                        frame->data[0], frame->linesize[0],
                        frame->data[1], frame->linesize[1],
                        frame->data[2], frame->linesize[2],
                        0, 0,
                        ctx->width, ctx->height,
                        ctx->width, ctx->height,
                        libyuv::kRotate0, ctx->yuv_fourcc);

    if (ret != 0) {
        if (ret == -1) {
            LOGE("ConvertToI420 failed because invalid parameter");
        } else {
            LOGE("ConvertToI420 failed don't know why.");
        }
        goto fail;
    }

    return JS_OK;

    fail:
    if (buffer) {
        av_free(buffer);
        frame->opaque = NULL;
    }
    return JS_ERR;
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