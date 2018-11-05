#include "js_egl_renderer.h"
#include <pthread.h>
#include <android/native_window_jni.h>

extern "C" {
#include "util/js_log.h"
#include "util/js_jni.h"
}

JSEglRenderer::JSEglRenderer(JSEventHandler *js_eventHandler) {
    m_js_event_handler = js_eventHandler;
    pthread_mutex_init(&m_mutex, NULL);
    pthread_cond_init(&m_start_render_cond, NULL);
    pthread_cond_init(&m_create_surface_cond, NULL);
}


JSEglRenderer::~JSEglRenderer() {
    pthread_mutex_destroy(&m_mutex);
    pthread_cond_destroy(&m_start_render_cond);
    pthread_cond_destroy(&m_create_surface_cond);

}


void JSEglRenderer::release_egl() {
    if (m_texture_yuv[TEXY]) {
        glDeleteTextures(3, m_texture_yuv);
    }
    if (m_g_program != 0) {
        glDeleteProgram(m_g_program);
        m_g_program = 0;
    }
    if (m_display) {
        eglMakeCurrent(m_display, 0, 0, 0);
    }
    if (m_context) {
        eglDestroyContext(m_display, m_context);
        m_context = NULL;
    }
    if (m_surface) {
        eglDestroySurface(m_display, m_surface);
        m_surface = NULL;
    }
    if (m_display) {
        eglTerminate(m_display);
        m_display = NULL;
    }

}


void *render_thread(void *data) {
    pthread_setname_np(pthread_self(), __func__);
    JSEglRenderer *renderer = (JSEglRenderer *) data;
    pthread_mutex_lock(&renderer->m_mutex);
    if (!renderer->m_is_renderer_thread_running) {
        goto end;
    }
    if (renderer->egl_setup() == JS_ERR) {
        LOGE("JSEglRenderer failed to egl_setup");
        goto fail;
    }

    pthread_mutex_unlock(&renderer->m_mutex);

    while (1) {

        pthread_mutex_lock(&renderer->m_mutex);
        if (!renderer->m_is_renderer_thread_running) {
            break;
        }

        if (!renderer->m_is_start_render) {
            LOGE("wait_for_start_render");
            renderer->m_is_waiting_for_start_render = true;
            pthread_cond_wait(&renderer->m_start_render_cond, &renderer->m_mutex);
            renderer->m_is_waiting_for_start_render = false;
            if (!renderer->m_is_renderer_thread_running) {
                break;
            }
        }
        if (!renderer->m_is_hold_surface) {
            LOGE("wait_for_create_surface");
            renderer->m_is_waiting_for_create_surface = true;
            pthread_cond_wait(&renderer->m_create_surface_cond, &renderer->m_mutex);
            renderer->m_is_waiting_for_create_surface = false;
            if (!renderer->m_is_renderer_thread_running) {
                break;
            }
        }

        if (renderer->m_is_need_update_surface) {

            if (renderer->m_surface) {// already exits surface.do destroy.
                eglMakeCurrent(renderer->m_display, 0, 0, 0);
                eglDestroySurface(renderer->m_display, renderer->m_surface);
                renderer->m_surface = NULL;
                glDeleteTextures(3, renderer->m_texture_yuv);
                glDeleteProgram(renderer->m_g_program);
                memset(renderer->m_texture_yuv, 0, sizeof(renderer->m_texture_yuv));
                renderer->m_g_program = 0;
            }


            int ret = ANativeWindow_setBuffersGeometry(
                    renderer->m_native_window,//fixme picture widht / height ??
                    renderer->m_picture_width,
                    renderer->m_picture_height,
                    renderer->m_format);
            if (ret) {
                LOGE("[EGL] ANativeWindow_setBuffersGeometry(format) returned error %d", ret);
                goto fail;
            }

            if (!(renderer->m_surface = eglCreateWindowSurface(renderer->m_display,//fixme 在这里崩溃过。
                                                               renderer->m_config,
                                                               renderer->m_native_window,
                                                               0))) {
                LOGE("eglCreateWindowSurface() returned error %d", eglGetError());
                goto fail;
            }

            if (!eglMakeCurrent(renderer->m_display, renderer->m_surface, renderer->m_surface,
                                renderer->m_context)) {
                LOGE("eglMakeCurrent() returned error %d", eglGetError());
                goto fail;
            }

            if (renderer->init_renderer() == JS_ERR) {
                LOGE("JSEglRenderer failed to init_renderer");
                goto fail;
            }

            renderer->m_is_need_update_surface = false;
        }

        if (renderer->m_is_window_size_changed) {
            glViewport(0, 0, renderer->m_picture_width,
                       renderer->m_picture_height);//fixme picture widht / height ??
            LOGD("glViewport m_window_width=%d,m_window_height=%d",
                 renderer->m_window_width,
                 renderer->m_window_height
            );
            renderer->m_is_window_size_changed = false;
        }
        pthread_mutex_unlock(&renderer->m_mutex);
        (*renderer->m_egl_buffer_queue_callback)(renderer->m_callback_data);
    }


    end:
    renderer->release_egl();
    if (renderer->m_native_window) {
        ANativeWindow_release(renderer->m_native_window);
        renderer->m_native_window = NULL;
    }
    pthread_mutex_unlock(&renderer->m_mutex);
    pthread_exit(0);

    fail:
    renderer->release_egl();
    renderer->m_js_event_handler->call_on_error(JS_EGL_RENDERER_SETUP_RENDERER_FAILED, 0, 0);
    LOGE("egl_setup or int_render failed");
    renderer->m_is_renderer_thread_running = false;
    renderer->m_is_start_render = false;
    pthread_mutex_unlock(&renderer->m_mutex);
    pthread_exit(0);
}


JS_RET JSEglRenderer::egl_setup() {
    LOGD("egl_setup");

    EGLint numConfigs;

    EGLDisplay display = NULL;
    EGLContext context = NULL;

    const EGLint configAttribs[] = {
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
            EGL_BLUE_SIZE, 4,
            EGL_GREEN_SIZE, 4,
            EGL_RED_SIZE, 4,
            EGL_ALPHA_SIZE, 4,
            EGL_NONE};

//    const EGLint configAttribs[] = {
//            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
//            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
//            EGL_BLUE_SIZE, 8,
//            EGL_GREEN_SIZE, 8,
//            EGL_RED_SIZE, 8,
//            EGL_ALPHA_SIZE, 8,
//            EGL_DEPTH_SIZE, 0,
//            EGL_STENCIL_SIZE, 0,
//            EGL_NONE};

    static const EGLint contextAttribs[] = {
            EGL_CONTEXT_CLIENT_VERSION, 2,
            EGL_NONE
    };

    if ((display = eglGetDisplay(EGL_DEFAULT_DISPLAY)) == EGL_NO_DISPLAY) {
        LOGE("eglGetDisplay() returned error %d", eglGetError());
        goto fail;
    }
    if (!eglInitialize(display, 0, 0)) {
        LOGE("eglInitialize() returned error %d", eglGetError());
        goto fail;
    }

    if (!eglChooseConfig(display, configAttribs, &m_config, 1, &numConfigs)) {
        LOGE("eglChooseConfig() returned error %d", eglGetError());
        goto fail;
    }

    if (!eglGetConfigAttrib(display, m_config, EGL_NATIVE_VISUAL_ID, &m_format)) {
        LOGE("eglGetConfigAttrib() returned error %d", eglGetError());
        goto fail;
    }

    if (!(context = eglCreateContext(display, m_config, 0, contextAttribs))) {
        LOGE("eglCreateContext() returned error %d", eglGetError());
        goto fail;
    }


    m_display = display;
    m_context = context;
    return JS_OK;

    fail:
    if (context) {
        eglDestroyContext(display, context);
    }

    if (display) {
        eglTerminate(display);
    }

    return JS_ERR;
}


JS_RET JSEglRenderer::init_renderer() {

    LOGD("init_renderer");
    JS_RET ret;
    GLint textureUniformY;
    GLint textureUniformU;
    GLint textureUniformV;

    gen_yuv_texture();

    ret = load_shader();

    if (ret == JS_ERR) {
        goto fail;
    }
    glUseProgram(m_g_program);

    textureUniformY = glGetUniformLocation(m_g_program, "SamplerY");
    textureUniformU = glGetUniformLocation(m_g_program, "SamplerU");
    textureUniformV = glGetUniformLocation(m_g_program, "SamplerV");
    glUniform1i(textureUniformY, 0);
    glUniform1i(textureUniformU, 1);
    glUniform1i(textureUniformV, 2);

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    return JS_OK;

    fail:
    if (m_texture_yuv[TEXY]) {
        glDeleteTextures(3, m_texture_yuv);
    }
    if (m_g_program != 0) {
        glDeleteProgram(m_g_program);
    }
    return JS_ERR;
}


GLuint JSEglRenderer::compile_shader(const char *pSource, GLenum shaderType) {
    GLuint shader = glCreateShader(shaderType);
    if (shader) {
        glShaderSource(shader, 1, &pSource, NULL);
        glCompileShader(shader);
        GLint compiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            GLint infoLen = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
            if (infoLen) {
                char *buf = (char *) malloc(infoLen);
                if (buf) {
                    glGetShaderInfoLog(shader, infoLen, NULL, buf);
                    LOGE("Could not compile shader %d: %s", shaderType, buf);
                    free(buf);
                }
            }
        }
    }
    return shader;
}


void JSEglRenderer::gen_yuv_texture() {

    glGenTextures(3, m_texture_yuv);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_texture_yuv[TEXY]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_texture_yuv[TEXU]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, m_texture_yuv[TEXV]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

JS_RET JSEglRenderer::load_shader() {
    GLint linkStatus = GL_FALSE;
    GLuint vertexShader = 0;
    GLuint fragmentShader = 0;

    m_g_program = glCreateProgram();
    if (!m_g_program) {
        goto fail;
    }
    vertexShader = compile_shader(gVertexShader, GL_VERTEX_SHADER);
    if (!vertexShader) {
        goto fail;
    }

    fragmentShader = compile_shader(gFragmentShader, GL_FRAGMENT_SHADER);
    if (!fragmentShader) {
        goto fail;
    }
    glAttachShader(m_g_program, vertexShader);
    glAttachShader(m_g_program, fragmentShader);

    glBindAttribLocation(m_g_program, ATTRIB_VERTEX, "position");
    glBindAttribLocation(m_g_program, ATTRIB_TEXTURE, "TexCoordIn");

    glLinkProgram(m_g_program);

    glGetProgramiv(m_g_program, GL_LINK_STATUS, &linkStatus);
    if (linkStatus != GL_TRUE) {
        GLint bufLength = 0;
        glGetProgramiv(m_g_program, GL_INFO_LOG_LENGTH, &bufLength);
        if (bufLength) {
            char *buf = (char *) malloc(bufLength);
            if (buf) {
                glGetProgramInfoLog(m_g_program, bufLength, NULL, buf);
                LOGE("Could not link program:%s", buf);
                free(buf);
            }
        }
        glDeleteProgram(m_g_program);
        m_g_program = 0;
        goto fail;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return JS_OK;

    fail:
    if (vertexShader)
        glDeleteShader(vertexShader);
    if (fragmentShader)
        glDeleteShader(fragmentShader);
    return JS_ERR;
}


void JSEglRenderer::render(AVFrame *frame) {

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_y_pixels = frame->data[0];
    m_u_pixels = frame->data[1];
    m_v_pixels = frame->data[2];

    glBindTexture(GL_TEXTURE_2D, m_texture_yuv[TEXY]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, frame->linesize[0], frame->height, 0,
                 GL_LUMINANCE, GL_UNSIGNED_BYTE, m_y_pixels);

    glBindTexture(GL_TEXTURE_2D, m_texture_yuv[TEXU]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, frame->linesize[1], frame->height / 2, 0,
                 GL_LUMINANCE, GL_UNSIGNED_BYTE, m_u_pixels);


    glBindTexture(GL_TEXTURE_2D, m_texture_yuv[TEXV]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, frame->linesize[2], frame->height / 2, 0,
                 GL_LUMINANCE, GL_UNSIGNED_BYTE, m_v_pixels);


    glVertexAttribPointer(ATTRIB_VERTEX, 2, GL_FLOAT, 0, 0, squareVertices);
    glEnableVertexAttribArray(ATTRIB_VERTEX);

    glVertexAttribPointer(ATTRIB_TEXTURE, 2, GL_FLOAT, 0, 0, coordVertices);
    glEnableVertexAttribArray(ATTRIB_TEXTURE);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    if (!eglSwapBuffers(m_display, m_surface)) {
        LOGW("eglSwapBuffers() returned error %d", eglGetError());

    }
}

void JSEglRenderer::set_egl_buffer_queue_callback(egl_buffer_queue_callback callback, void *data) {

    m_callback_data = data;
    m_egl_buffer_queue_callback = callback;

}


void JSEglRenderer::create_renderer_thread() {
    LOGD("create_renderer_thread...");
    pthread_mutex_lock(&m_mutex);
    m_is_renderer_thread_running = true;
    pthread_create(&m_render_tid, NULL, render_thread, this);
    pthread_mutex_unlock(&m_mutex);
}

void JSEglRenderer::destroy_renderer_thread() {
    LOGD("destroy_renderer_thread1...");
    if (!m_is_renderer_thread_running) {
        return;
    }
    pthread_mutex_lock(&m_mutex);
    m_is_renderer_thread_running = false;
    if (m_is_waiting_for_start_render) {
        pthread_cond_signal(&m_start_render_cond);
    } else if (m_is_waiting_for_create_surface) {
        pthread_cond_signal(&m_create_surface_cond);
    }
    pthread_mutex_unlock(&m_mutex);
    pthread_join(m_render_tid, NULL);
    LOGD("destroy_renderer_thread2...");
}

void JSEglRenderer::create_surface(jobject surface) {
    LOGD("create_surface");

    pthread_mutex_lock(&m_mutex);
    m_native_window = ANativeWindow_fromSurface(js_jni_get_env(NULL), surface);
    if (m_is_waiting_for_create_surface) {
        pthread_cond_signal(&m_create_surface_cond);
    }
    m_is_hold_surface = true;
    m_is_need_update_surface = true;
    pthread_mutex_unlock(&m_mutex);
}

void JSEglRenderer::window_size_changed(int width, int height) {

    pthread_mutex_lock(&m_mutex);
    LOGD("m_is_window_size_changed m_window_width=%d,m_window_height=%d,width=%d,height=%d",
         m_window_width,
         m_window_height,
         width,
         height);
    if (m_window_width == width && m_window_height == height) {
        pthread_mutex_unlock(&m_mutex);
        return;
    }
    m_is_window_size_changed = true;
    m_window_width = width;
    m_window_height = height;

    pthread_mutex_unlock(&m_mutex);
}


void JSEglRenderer::destroy_surface() {
    LOGD("destroy_surface1");

    pthread_mutex_lock(&m_mutex);
    ANativeWindow_release(m_native_window);
    m_native_window = NULL;
    m_is_hold_surface = false;
    pthread_mutex_unlock(&m_mutex);
    while (!m_is_waiting_for_start_render && !m_is_waiting_for_create_surface);
    LOGD("destroy_surface2");
}


void JSEglRenderer::start_render(int picture_width, int picture_height) {
    LOGD("start_render1...");
    if (!m_is_renderer_thread_running) {
        return;
    }
    pthread_mutex_lock(&m_mutex);
    m_picture_width = picture_width;
    m_picture_height = picture_height;
    m_is_start_render = true;
    pthread_cond_signal(&m_start_render_cond);
    pthread_mutex_unlock(&m_mutex);
    LOGD("start_render2...");
}

void JSEglRenderer::stop_render() {
    LOGD("stop_render1...");
    if (!m_is_renderer_thread_running) {
        return;
    }
    pthread_mutex_lock(&m_mutex);
    m_is_start_render = false;
    pthread_mutex_unlock(&m_mutex);
    while (!m_is_waiting_for_create_surface && !m_is_waiting_for_start_render);
    LOGD("stop_render2...");
}

