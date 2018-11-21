#ifndef JS_MEDIA_CONVERTER_H
#define JS_MEDIA_CONVERTER_H

#include <stdint.h>
#include "js_constant.h"
#include "decoder/js_media_hw_decoder_context.h"

extern "C" {
#include "libavutil/frame.h"
}

#define   DST_FORMAT                         AV_SAMPLE_FMT_S16
#define   DST_BITS_PER_SAMPLE                16
//#define   DST_LAYOUT                         AV_CH_LAYOUT_STEREO
//#define   DST_CHANNEL                        2
//#define   DST_SAMPLE_RATE                    44100


unsigned int convert_simple_format_to_S16(uint8_t *audio_buf, AVFrame *frame);

JS_RET convert_color_format(
        JSMediaDecoderContext *ctx,
        uint8_t *data,
        size_t size,
        AVFrame *frame);


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