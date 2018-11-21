#ifndef JS_MEDIA_DECODER_H
#define JS_MEDIA_DECODER_H

#include "js_media_hw_decoder_context.h"
#include "js_constant.h"
#include "event/js_event_handler.h"

extern "C" {
#include "libavformat/avformat.h"
}

class JSMediaDecoder;

typedef AVFrame *(JSMediaDecoder::*DECODE_PACKET_FUNC)(AVPacket *avpkt);

typedef void (JSMediaDecoder::*FLUSH_FUNC)();


class JSMediaDecoder {

public:

    JSMediaDecoder(JSEventHandler *m_js_event_handler);

    ~JSMediaDecoder();

    void set_decoder_type(const char *decoder_type);

    JS_RET (JSMediaDecoder::*m_create_decoder_by_av_stream)(AVStream *av_stream);

    DECODE_PACKET_FUNC m_decode_video_packet;
    DECODE_PACKET_FUNC m_decode_audio_packet;

    FLUSH_FUNC m_video_flush;
    FLUSH_FUNC m_audio_flush;

private:

    char *m_decoder_type;

    JSEventHandler *m_js_event_handler = NULL;

    AVFrame *m_video_reuse_frame = NULL;
    AVFrame *m_audio_reuse_frame = NULL;

    JSMediaDecoderContext *m_video_hw_dec_ctx = NULL, *m_audio_hw_dec_ctx = NULL;
    void *m_video_hw_dec = NULL, *m_audio_hw_dec = NULL;

    AVCodecContext *m_video_sw_dec_ctx = NULL, *m_audio_sw_dec_ctx = NULL;
    AVCodec *m_video_sw_dec = NULL, *m_audio_sw_dec = NULL;

    AVStream *m_audio_stream = NULL;
    AVStream *m_video_stream = NULL;

    void update_funcs_by_decode_type(const AVMediaType media_type, const char *decoder_type);

    JS_RET create_video_hw_decoder();

    JS_RET create_audio_hw_decoder();

    JS_RET create_sw_decoder_by_av_stream(AVStream *av_stream);

    JS_RET create_hw_decoder_by_av_stream(AVStream *av_stream);

    JS_RET process_hw_decode(AVPacket *avpkt,
                             AVFrame *frame);


    AVFrame *decode_audio_packet_with_hw(AVPacket *avpkt);

    AVFrame *decode_audio_packet_with_sw(AVPacket *avpkt);

    AVFrame *decode_video_packet_with_hw(AVPacket *avpkt);

    AVFrame *decode_video_packet_with_sw(AVPacket *avpkt);

    inline AVFrame *decode_packet_with_sw_internal(AVCodecContext *avctx, AVPacket *avpkt,
                                                   AVFrame *frame);

    void audio_sw_decoder_flush();

    void audio_hw_decoder_flush();

    void video_sw_decoder_flush();

    void video_hw_decoder_flush();
};

#endif //JS_MEDIA_DECODER_H
