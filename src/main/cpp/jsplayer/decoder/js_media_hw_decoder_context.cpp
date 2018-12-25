#include "js_media_hw_decoder_context.h"
#include "converter/js_media_converter.h"
#include "js_mediacodec_def.h"

extern "C" {
#include "js_java_mediacodec_wrapper.h"
#include "util/js_log.h"
}


JSMediaDecoderContext::JSMediaDecoderContext() {

}

JSMediaDecoderContext::~JSMediaDecoderContext() {

    if (codec_name) {
        av_freep(&codec_name);
    }

    if (format) {
        js_MediaFormat_delete(format);
    }

    if (frame_buf) {
        av_frame_free(&frame_buf);
    }

}


JS_RET JSMediaDecoderContext::update_pix_fmt() {

    if (!strcmp(codec_name, "OMX.k3.video.decoder.avc") &&
        color_format == OMX_COLOR_FormatYCbYCr) {
        color_format = color_format = OMX_TI_COLOR_FormatYUV420PackedSemiPlanar;
    }

    if (!strcmp(codec_name, "OMX.MTK.VIDEO.DECODER.AVC") &&
        color_format == 0x103) {// maxhub 解码颜色格式
        color_format = color_format = OMX_COLOR_FormatYUV420PackedPlanar;
    }
//    if (!strcmp(codec_name, "OMX.hisi.video.decoder") &&
//        color_format == OMX_COLOR_FormatYUV411Planar) {// 海信VIDAA-47 解码颜色格式
//        color_format = color_format = OMX_COLOR_FormatYUV420Planar;
//    }
    //others.

    for (int i = 0; i < FF_ARRAY_ELEMS(color_formats); i++) {
        if (color_formats[i].color_format == color_format) {
            pix_fmt = color_formats[i].pix_fmt;
            yuv_fourcc = color_formats[i].yuv_fourcc;

            return JS_OK;
        }
    }

    return JS_ERR;
}


JS_RET JSMediaDecoderContext::update_media_decoder_context() {
    JS_RET ret;

    if (frame_buf) {
        av_frame_free(&frame_buf);
    }

    frame_buf = av_frame_alloc();

    if (!frame_buf) {
        return JS_ERR;
    }


    const char *str_format = NULL;
    js_MediaFormat_getInt32(format, JS_MEDIAFORMAT_KEY_WIDTH, &width);
    js_MediaFormat_getInt32(format, JS_MEDIAFORMAT_KEY_HEIGHT, &height);
    js_MediaFormat_getInt32(format, JS_MEDIAFORMAT_KEY_COLOR_FORMAT, &color_format);
    js_MediaFormat_getInt32(format, JS_MEDIAFORMAT_KEY_STRIDE, &stride);
    js_MediaFormat_getInt32(format, JS_MEDIAFORMAT_KEY_SLICE_HEIGHT, &slice_height);
    js_MediaFormat_getInt32(format, JS_MEDIAFORMAT_KEY_CROP_LEFT, &crop_left);
    js_MediaFormat_getInt32(format, JS_MEDIAFORMAT_KEY_CROP_TOP, &crop_top);
    js_MediaFormat_getInt32(format, JS_MEDIAFORMAT_KEY_CROP_RIGHT, &crop_right);
    js_MediaFormat_getInt32(format, JS_MEDIAFORMAT_KEY_CROP_BOTTOM, &crop_bottom);


    frame_buf->width = width;
    frame_buf->height = height;
    frame_buf->format = DEFAULT_AV_PIX_FMT;

    //todo how to set align value.
    if (av_frame_get_buffer(frame_buf, 0) != 0) {
        LOGE("%s av_frame_get_buffer failed", __func__);
        return JS_ERR;
    }

    ret = update_pix_fmt();
    if (ret != JS_OK) {
        LOGE("%s output color format 0x%x (value=%d) is not supported", __func__,
             color_format, color_format);
        return JS_ERR;
    } else {
        LOGD("%s output color format is 0x%x (value=%d) pix_fmt=%d yuv_fourcc=%d", __func__,
             color_format,
             color_format,
             pix_fmt, yuv_fourcc);
    }

    str_format = js_MediaFormat_toString(format);
    if (str_format) {
        LOGD("%s output MediaFormat changed to %s", __func__, str_format);
        av_freep(&str_format);
    }

    return JS_OK;
}