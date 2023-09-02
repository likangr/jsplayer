#ifndef JS_MEDIA_DECODER_CONTEXT_H
#define JS_MEDIA_DECODER_CONTEXT_H

#include "js_constant.h"
#include <sys/types.h>

extern "C" {
#include "libavutil/frame.h"
#include "libavutil/pixfmt.h"
}


class JSMediaDecoderContext {


public:

    JSMediaDecoderContext();

    ~JSMediaDecoderContext();

    JS_RET update_media_decoder_context();

    //video

    int width = 0;
    int height = 0;
    int color_format = 0;
    int stride = 0;
    int slice_height = 0;
    int crop_top = 0;
    int crop_left = 0;
    int crop_bottom = 0;
    int crop_right = 0;

    int pix_fmt = AV_PIX_FMT_NONE;

    char *codec_name = NULL;
    char mime_type[128] = {0};
    int profile = 0;
    int level = 0;
    void *format = NULL;
    u_int32_t yuv_fourcc = 0;
    size_t nal_size = 0;

private:
    JS_RET update_pix_fmt();
};


#endif //JS_MEDIA_DECODER_CONTEXT_H
