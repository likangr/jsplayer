#ifndef JS_MEDIA_DECODER_H
#define JS_MEDIA_DECODER_H

#include "js_media_decoder_context.h"
#include "js_constant.h"

extern "C" {
#include "libavformat/avformat.h"
}

class JSMediaDecoder;
typedef AVFrame *(JSMediaDecoder::*DECODE_PACKET_FUNC)(AVPacket *avpkt);

class JSMediaDecoder {

public:
    JSMediaDecoder();

    ~JSMediaDecoder();

    JS_RET create_decoder(AVStream *av_stream);


    DECODE_PACKET_FUNC m_decode_video_packet;
    DECODE_PACKET_FUNC m_decode_audio_packet;

    void set_decode_packet_type(const char *decode_type);

    JS_RET flush();

private:

    bool m_audio_hw_dec_available = false;
    bool m_video_hw_dec_available = false;

    JSMediaDecoderContext *m_video_hw_dec_ctx = NULL, *m_audio_hw_dec_ctx = NULL;
    void *m_video_hw_dec = NULL, *m_audio_hw_dec = NULL;

    AVCodecContext *m_video_sw_dec_ctx = NULL, *m_audio_sw_dec_ctx = NULL;
    AVCodec *m_video_sw_dec = NULL, *m_audio_sw_dec = NULL;

    AVStream *m_audio_stream = NULL;
    AVStream *m_video_stream = NULL;


    JS_RET create_hw_decoder(AVStream *av_stream);

    JS_RET create_sw_decoder(AVStream *av_stream);

    JS_RET process_hw_decode(AVPacket *avpkt,
                             AVFrame *frame);

    AVFrame *decode_audio_packet_with_hw(AVPacket *avpkt);

    AVFrame *decode_audio_packet_with_sw(AVPacket *avpkt);

    AVFrame *decode_video_packet_with_hw(AVPacket *avpkt);

    AVFrame *decode_video_packet_with_sw(AVPacket *avpkt);

};

#endif //JS_MEDIA_DECODER_H
