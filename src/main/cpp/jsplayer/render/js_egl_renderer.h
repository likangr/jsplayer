#ifndef JS_EGL_RENDERER_H
#define JS_EGL_RENDERER_H

#include "event/js_event_handler.h"
#include "js_constant.h"
#include <GLES2/gl2.h>
#include <EGL/egl.h>

extern "C" {
#include "libavutil/frame.h"
}

static const char gFragmentShader[] =
        "varying lowp vec2 TexCoordOut;                        \n"
                "uniform sampler2D SamplerY;                           \n"
                "uniform sampler2D SamplerU;                           \n"
                "uniform sampler2D SamplerV;                           \n"
                "void main(void)                                       \n"
                "{                                                     \n"
                "mediump vec3 yuv;                                     \n"
                "lowp vec3 rgb;                                        \n"
                "yuv.x = texture2D(SamplerY, TexCoordOut).r;           \n"
                "yuv.y = texture2D(SamplerU, TexCoordOut).r - 0.5;     \n"
                "yuv.z = texture2D(SamplerV, TexCoordOut).r - 0.5;     \n"
                "rgb = mat3( 1,       1,         1,                    \n"
                "0,       -0.39465,  2.03211,                          \n"
                "1.13983, -0.58060,  0) * yuv;                         \n"
                "gl_FragColor = vec4(rgb, 1);                          \n"
                "}                                                     \n";


static const char gVertexShader[] =
        "attribute vec4 position;              \n"
                "attribute vec2 TexCoordIn;    \n"
                "varying vec2 TexCoordOut;     \n"
                "void main(void)               \n"
                "{                             \n"
                "gl_Position = position;       \n"
                "TexCoordOut = TexCoordIn;     \n"
                "}                             \n";


static const GLfloat squareVertices[] = {-1.0f, -1.0f, 1.0f, -1.0f, -1.0f,
                                         1.0f, 1.0f, 1.0f,};

static const GLfloat coordVertices[] = {0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f,
                                        1.0f, 0.0f,};

enum AttribEnum {
    ATTRIB_VERTEX,
    ATTRIB_TEXTURE,
};

enum TextureType {
    TEXY = 0,
    TEXU,
    TEXV,
};

typedef void (*egl_buffer_queue_callback)(void *data);

class JSEglRenderer {

public:


    JSEglRenderer(JSEventHandler *js_eventHandler);

    ~JSEglRenderer();

    void create_renderer_thread();

    void create_surface(jobject surface);

    void destroy_surface();

    JS_RET egl_setup();

    JS_RET init_renderer();

    void start_render(int picture_width, int picture_height);

    void stop_render();

    void destroy_renderer_thread();

    void release_egl();

    void window_size_changed(int width, int height);

    void render(AVFrame *frame);

    void set_egl_buffer_queue_callback(egl_buffer_queue_callback callback, void *data);

    volatile bool m_is_window_size_changed = false;
    volatile int m_window_width = 0;
    volatile int m_window_height = 0;
    volatile int m_picture_width = 0;
    volatile int m_picture_height = 0;

    JSEventHandler *m_js_event_handler = NULL;

    egl_buffer_queue_callback m_egl_buffer_queue_callback;
    void *m_callback_data;

    pthread_mutex_t m_mutex;
    pthread_cond_t m_start_render_cond;
    pthread_cond_t m_create_surface_cond;


    volatile bool m_is_start_render = false;
    volatile bool m_is_waiting_for_start_render = false;
    volatile bool m_is_waiting_for_create_surface = false;
    volatile bool m_is_renderer_thread_running = false;

    pthread_t m_render_tid;

    volatile bool m_is_need_update_surface = false;
    volatile bool m_is_hold_surface = false;

    EGLDisplay m_display = NULL;
    EGLSurface m_surface = NULL;
    EGLContext m_context = NULL;
    ANativeWindow *m_native_window = NULL;

    EGLint m_format;
    EGLConfig m_config;

    GLuint m_texture_yuv[3] = {0};

    GLuint m_g_program = 0;

private:
    uint8_t *m_y_pixels = NULL;
    uint8_t *m_u_pixels = NULL;
    uint8_t *m_v_pixels = NULL;

    void gen_yuv_texture();

    GLuint compile_shader(const char *pSource, GLenum shaderType);

    JS_RET load_shader();


};

void *render_thread(void *data);


#endif //JS_EGL_RENDERER_H
