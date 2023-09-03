#include "js_egl_renderer.h"
#include <pthread.h>
#include <android/native_window_jni.h>

extern "C" {
#include "util/js_log.h"
#include "util/js_jni.h"
}

JSEglRenderer::JSEglRenderer(JSEventHandler *js_eventHandler) {
    m_js_event_handler = js_eventHandler;
    pthread_mutex_init(m_mutex, NULL);
    pthread_cond_init(m_start_render_cond, NULL);
    pthread_cond_init(m_create_surface_cond, NULL);
}


JSEglRenderer::~JSEglRenderer() {
    pthread_mutex_destroy(m_mutex);
    pthread_cond_destroy(m_start_render_cond);
    pthread_cond_destroy(m_create_surface_cond);
    if (m_native_window) {
        ANativeWindow_release(m_native_window);
    }
}


void JSEglRenderer::release_egl() {
    if (m_texture_yuv[0]) {
        glDeleteTextures(3, m_texture_yuv);
        memset(m_texture_yuv, 0, sizeof(m_texture_yuv));
    }
    if (m_g_program) {
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
//    eglReleaseThread();fixme
}

int JSEglRenderer::query_surface_width() {
    EGLint width = 0;
    if (!eglQuerySurface(m_display, m_surface, EGL_WIDTH, &width)) {
        LOGE("%s [EGL] eglQuerySurface(EGL_WIDTH) returned error %d", __func__, eglGetError());
        return 0;
    }
    return width;
}

int JSEglRenderer::query_surface_height() {
    EGLint height = 0;
    if (!eglQuerySurface(m_display, m_surface, EGL_HEIGHT, &height)) {
        LOGE("%s [EGL] eglQuerySurface(EGL_HEIGHT) returned error %d", __func__, eglGetError());
        return 0;
    }
    return height;
}


void *render_thread(void *data) {
    pthread_setname_np(pthread_self(), __func__);
    JSEglRenderer *renderer = (JSEglRenderer *) data;
    pthread_mutex_lock(renderer->m_mutex);
    if (!renderer->m_is_renderer_thread_running) {
        goto end;
    }
    if (renderer->egl_setup() != JS_OK) {
        LOGE("%s JSEglRenderer failed to egl_setup", __func__);
        goto fail;
    }

    pthread_mutex_unlock(renderer->m_mutex);

    while (1) {

        pthread_mutex_lock(renderer->m_mutex);
        if (!renderer->m_is_renderer_thread_running) {
            break;
        }

        if (!renderer->m_is_start_render) {
            LOGD("%s wait_for_start_render", __func__);
            renderer->m_is_waiting_for_start_render = true;
            pthread_cond_wait(renderer->m_start_render_cond, renderer->m_mutex);
            renderer->m_is_waiting_for_start_render = false;
            if (!renderer->m_is_renderer_thread_running) {
                break;
            }
        }
        if (!renderer->m_is_hold_surface) {
            LOGD("%s wait_for_create_surface", __func__);
            renderer->m_is_waiting_for_create_surface = true;
            pthread_cond_wait(renderer->m_create_surface_cond, renderer->m_mutex);
            renderer->m_is_waiting_for_create_surface = false;
            if (!renderer->m_is_renderer_thread_running) {
                break;
            }
        }

        if (renderer->m_has_new_surface) {

            if (renderer->m_surface) {
                eglMakeCurrent(renderer->m_display, 0, 0, 0);
                glDeleteTextures(3, renderer->m_texture_yuv);
                memset(renderer->m_texture_yuv, 0, sizeof(renderer->m_texture_yuv));
                glDeleteProgram(renderer->m_g_program);
                renderer->m_g_program = 0;
                eglDestroySurface(renderer->m_display, renderer->m_surface);
                renderer->m_surface = NULL;
            }


            //fixme is need ANativeWindow_setBuffersGeometry?
//            int32_t width = ANativeWindow_getWidth(renderer->m_native_window);
//            int32_t height = ANativeWindow_getWidth(renderer->m_native_window);
//            int ret = ANativeWindow_setBuffersGeometry(renderer->m_native_window, width, height,
//                                                       renderer->m_format);
//            if (ret) {
//                LOGE("%s ANativeWindow_setBuffersGeometry() returned error %d", __func__,ret);
//                goto fail;
//            }


            if (!(renderer->m_surface = eglCreateWindowSurface(renderer->m_display,
                                                               renderer->m_config,
                                                               renderer->m_native_window,
                                                               0))) {
                LOGE("%s eglCreateWindowSurface() returned error %d", __func__, eglGetError());
                goto fail;
            }

            if (!eglMakeCurrent(renderer->m_display, renderer->m_surface, renderer->m_surface,
                                renderer->m_context)) {
                LOGE("%s eglMakeCurrent() returned error %d", __func__, eglGetError());
                goto fail;
            }

            if (renderer->init_renderer() != JS_OK) {
                LOGE("%s JSEglRenderer failed to init_renderer", __func__);
                goto fail;
            }

            renderer->m_has_new_surface = false;
        }


//        if (renderer->m_is_window_size_changed) {
//            LOGD("%s glViewport m_window_width=%d,m_window_height=%d",
//                 renderer->m_window_width,
//                 renderer->m_window_height);
//
//            glViewport(0, 0, renderer->m_window_width,
//                       renderer->m_window_height);
//            renderer->m_is_window_size_changed = false;
//        }

        (*renderer->egl_buffer_queue_callback)(renderer->m_egl_buffer_queue_callback_data);
        pthread_mutex_unlock(renderer->m_mutex);
    }


    end:
    renderer->release_egl();
    if (renderer->m_native_window) {
        ANativeWindow_release(renderer->m_native_window);
        renderer->m_native_window = NULL;
    }
    pthread_mutex_unlock(renderer->m_mutex);
    pthread_exit(0);

    fail:
    renderer->release_egl();
    renderer->m_js_event_handler->call_on_error(JS_ERR_EGL_RENDERER_SETUP_RENDERER_FAILED, 0,
                                                0);
    LOGE("%s egl thread exit unexpected.", __func__);
    renderer->m_is_renderer_thread_running = false;
    renderer->m_is_start_render = false;
    pthread_mutex_unlock(renderer->m_mutex);
    pthread_exit(0);
}


JS_RET JSEglRenderer::egl_setup() {
    LOGD("%s egl_setup...", __func__);

    EGLint numConfigs;

    EGLDisplay display = NULL;
    EGLContext context = NULL;

    static const EGLint configAttribs[] = {
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_BLUE_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_RED_SIZE, 8,
            EGL_NONE
    };

    static const EGLint contextAttribs[] = {
            EGL_CONTEXT_CLIENT_VERSION, 2,
            EGL_NONE
    };

    if ((display = eglGetDisplay(EGL_DEFAULT_DISPLAY)) == EGL_NO_DISPLAY) {
        LOGE("%s eglGetDisplay() returned error %d", __func__, eglGetError());
        goto fail;
    }
    if (!eglInitialize(display, 0, 0)) {
        LOGE("%s eglInitialize() returned error %d", __func__, eglGetError());
        goto fail;
    }

    if (!eglChooseConfig(display, configAttribs, &m_config, 1, &numConfigs)) {
        LOGE("%s eglChooseConfig() returned error %d", __func__, eglGetError());
        goto fail;
    }

    if (!eglGetConfigAttrib(display, m_config, EGL_NATIVE_VISUAL_ID, &m_format)) {
        LOGE("%s eglGetConfigAttrib() returned error %d", __func__, eglGetError());
        goto fail;
    }

    if (!(context = eglCreateContext(display, m_config, 0, contextAttribs))) {
        LOGE("%s eglCreateContext() returned error %d", __func__, eglGetError());
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

    LOGD("%s init_renderer...", __func__);

    JS_RET ret;

    ret = use_configured_program();
    if (ret != JS_OK) {
        return JS_ERR;
    }

    GLint samplers[3];
    samplers[0] = glGetUniformLocation(m_g_program, "SamplerY");
    samplers[1] = glGetUniformLocation(m_g_program, "SamplerU");
    samplers[2] = glGetUniformLocation(m_g_program, "SamplerV");

    glGenTextures(3, m_texture_yuv);
    for (int i = 0; i < 3; i++) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, m_texture_yuv[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glUniform1i(samplers[i], i);
    }

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glDisable(GL_DEPTH_TEST);
    return JS_OK;
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
                    LOGE("%s Could not compile shader %d: %s", __func__, shaderType, buf);
                    free(buf);
                }
            }
        }
    }
    return shader;
}


JS_RET JSEglRenderer::use_configured_program() {
    GLint linkStatus = GL_FALSE;
    GLuint program = 0;
    GLuint vertexShader = 0;
    GLuint fragmentShader = 0;

    program = glCreateProgram();
    if (!program) {
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
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);

    glBindAttribLocation(program, ATTRIB_VERTEX, "position");
    glBindAttribLocation(program, ATTRIB_TEXTURE, "TexCoordIn");

    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    if (linkStatus != GL_TRUE) {
        GLint bufLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
        if (bufLength) {
            char *buf = (char *) malloc(bufLength);
            if (buf) {
                glGetProgramInfoLog(program, bufLength, NULL, buf);
                LOGE("%s Could not link program:%s", __func__, buf);
                free(buf);
            }
        }
        goto fail;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    m_g_program = program;
    glUseProgram(m_g_program);
    return JS_OK;

    fail:
    if (vertexShader)
        glDeleteShader(vertexShader);
    if (fragmentShader)
        glDeleteShader(fragmentShader);
    if (program) {
        glDeleteProgram(program);
    }
    return JS_ERR;
}


void JSEglRenderer::render(AVFrame *frame) {

    const int heights[3] = {frame->height, frame->height / 2, frame->height / 2};

    m_window_width = query_surface_width();
    m_window_height = query_surface_height();

    if (m_window_width != frame->width || m_window_height != frame->height) {
        int ret = ANativeWindow_setBuffersGeometry(m_native_window,
                                                   frame->width,
                                                   frame->height,
                                                   m_format);

        if (ret) {
            LOGE("%s ANativeWindow_setBuffersGeometry(format) returned error %d", __func__, ret);
            goto failed;
        }

        m_window_width = query_surface_width();
        m_window_height = query_surface_height();
    }


    glViewport(0, 0, m_window_width,
               m_window_height);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);


    for (int i = 0; i < 3; i++) {
        glBindTexture(GL_TEXTURE_2D, m_texture_yuv[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, frame->linesize[i], heights[i], 0,
                     GL_LUMINANCE, GL_UNSIGNED_BYTE, frame->data[i]);
    }

    glVertexAttribPointer(ATTRIB_VERTEX, 2, GL_FLOAT, 0, 0, squareVertices);
    glEnableVertexAttribArray(ATTRIB_VERTEX);

    glVertexAttribPointer(ATTRIB_TEXTURE, 2, GL_FLOAT, 0, 0, coordVertices);
    glEnableVertexAttribArray(ATTRIB_TEXTURE);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    if (!eglSwapBuffers(m_display, m_surface)) {
        LOGE("%s eglSwapBuffers() returned error %d", __func__, eglGetError());
        goto failed;
    }

    return;

    failed:
    m_js_event_handler->call_on_error(JS_ERR_EGL_RENDERER_RENDER_FAILED, 0, 0);
}

void JSEglRenderer::set_egl_buffer_queue_callback(EGL_BUFFER_QUEUE_CALLBACK callback, void *data) {
    m_egl_buffer_queue_callback_data = data;
    egl_buffer_queue_callback = callback;
}


void JSEglRenderer::create_renderer_thread() {
    LOGD("%s create_renderer_thread...", __func__);
    pthread_mutex_lock(m_mutex);
    if (m_is_renderer_thread_running) {
        return;
    }
    m_is_renderer_thread_running = true;
    pthread_create(&m_render_tid, NULL, render_thread, this);
    pthread_mutex_unlock(m_mutex);
}

void JSEglRenderer::destroy_renderer_thread() {
    LOGD("%s destroy_renderer_thread1...", __func__);
    pthread_mutex_lock(m_mutex);
    if (!m_is_renderer_thread_running) {
        return;
    }
    m_is_renderer_thread_running = false;
    if (m_is_waiting_for_start_render) {
        pthread_cond_signal(m_start_render_cond);
    } else if (m_is_waiting_for_create_surface) {
        pthread_cond_signal(m_create_surface_cond);
    }
    pthread_mutex_unlock(m_mutex);
    pthread_join(m_render_tid, NULL);
    LOGD("%s destroy_renderer_thread2...", __func__);
}

void JSEglRenderer::create_surface(jobject surface) {
    LOGD("%s create_surface", __func__);

    pthread_mutex_lock(m_mutex);
    m_native_window = ANativeWindow_fromSurface(js_jni_get_env(NULL), surface);
    if (m_is_waiting_for_create_surface) {
        pthread_cond_signal(m_create_surface_cond);
    }
    m_is_hold_surface = true;
    m_has_new_surface = true;
    pthread_mutex_unlock(m_mutex);
}

//void JSEglRenderer::window_size_changed(int width, int height) {
//
//    pthread_mutex_lock(m_video_buffering_mutex);
//    LOGD("%s m_is_window_size_changed m_window_width=%d,m_window_height=%d,width=%d,height=%d",
//       __func__,
//          m_window_width,
//         m_window_height,
//         width,
//         height);
//    if (m_window_width == width && m_window_height == height) {
//        pthread_mutex_unlock(m_video_buffering_mutex);
//        return;
//    }
//    m_is_window_size_changed = true;
//    m_window_width = width;
//    m_window_height = height;
//
//    pthread_mutex_unlock(m_video_buffering_mutex);
//}


void JSEglRenderer::destroy_surface() {
    LOGD("%s destroy_surface1", __func__);

    pthread_mutex_lock(m_mutex);
    ANativeWindow_release(m_native_window);
    m_native_window = NULL;
    m_is_hold_surface = false;
    pthread_mutex_unlock(m_mutex);
    while (!m_is_waiting_for_start_render && !m_is_waiting_for_create_surface);
    LOGD("%s destroy_surface2", __func__);
}


void JSEglRenderer::start_render() {
    LOGD("%s start_render1...", __func__);
    pthread_mutex_lock(m_mutex);
    if (!m_is_renderer_thread_running) {
        return;
    }
    m_is_start_render = true;
    pthread_cond_signal(m_start_render_cond);
    pthread_mutex_unlock(m_mutex);
    LOGD("%s start_render2...", __func__);
}

void JSEglRenderer::stop_render() {
    LOGD("%s stop_render1...", __func__);
    pthread_mutex_lock(m_mutex);
    if (!m_is_renderer_thread_running) {
        return;
    }
    m_is_start_render = false;
    pthread_mutex_unlock(m_mutex);
    while (!m_is_waiting_for_create_surface && !m_is_waiting_for_start_render);
    LOGD("%s stop_render2...", __func__);
}