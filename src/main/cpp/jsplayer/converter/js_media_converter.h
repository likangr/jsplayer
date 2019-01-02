#ifndef JS_MEDIA_CONVERTER_H
#define JS_MEDIA_CONVERTER_H

#include <stdint.h>
#include "js_constant.h"
#include "decoder/js_media_hw_decoder_context.h"

extern "C" {
#include "libavutil/frame.h"
#include "decoder/js_java_mediacodec_wrapper.h"
#include "util/js_log.h"
#include "libavutil/imgutils.h"
#include "libswresample/swresample.h"
}

#define   DEFAULT_AV_PIX_FMT                            AV_PIX_FMT_YUV420P
#define   DEFAULT_AV_SAMPLE_FMT                         AV_SAMPLE_FMT_S16
#define   DST_BITS_PER_SAMPLE                           16
#define   DST_BYTES_PER_SAMPLE                          2


class JSMediaCoverter {
public:
    SwrContext *m_audio_swr_ctx = NULL;

    int64_t m_out_ch_layout = 0;
    int m_out_channel = 0;
    enum AVSampleFormat m_out_sample_fmt = AV_SAMPLE_FMT_NONE;
    int m_out_sample_rate = 0;
    int m_out_bytes_per_sample = 0;
    int64_t m_in_ch_layout = 0;
    int m_in_channel = 0;
    enum AVSampleFormat m_in_sample_fmt = AV_SAMPLE_FMT_NONE;
    int m_in_sample_rate = 0;

    JS_RET init_audio_converter(struct SwrContext *s,
                                int64_t out_ch_layout,
                                int out_channel,
                                enum AVSampleFormat out_sample_fmt,
                                int out_sample_rate,
                                int64_t in_ch_layout,
                                int in_channel,
                                enum AVSampleFormat in_sample_fmt,
                                int in_sample_rate);

    void release_audio_converter();

    bool is_audio_converter_initialized();

    int convert_simple_format_to_S16(uint8_t *converted_pcm_data, AVFrame *frame);
};


JS_RET fill_video_frame(JSMediaDecoderContext *ctx,
                        JSMediaCodec *video_hw_dec,
                        AVStream *av_stream,
                        JSMediaCodecBufferInfo *info,
                        AVFrame *frame, uint8_t *data,
                        size_t size);


JS_RET convert_sps_pps(const uint8_t *p_buf, size_t i_buf_size,
                       uint8_t *p_out_buf, size_t i_out_buf_size,
                       size_t *p_sps_pps_size, size_t *p_nal_size);

/* Convert H.264 NAL format to annex b in-place */
typedef struct H264ConvertState {
    uint32_t nal_len;
    uint32_t nal_pos;
} H264ConvertState;

void convert_h264_to_annexb(uint8_t *p_buf, size_t i_len,
                            size_t i_nal_size,
                            H264ConvertState *state);

#endif //JS_MEDIA_CONVERTER_H