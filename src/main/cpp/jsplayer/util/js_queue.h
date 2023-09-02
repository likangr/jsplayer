#ifndef JS_QUEUE_H
#define JS_QUEUE_H

#include <deque>
#include <pthread.h>
#include <typeinfo>

extern "C" {
#include "util/js_log.h"
#include "libavutil/rational.h"
#include "libavcodec/avcodec.h"
}

using namespace std;

enum QUEUE_TYPE {
    QUEUE_TYPE_VIDEO = 0,
    QUEUE_TYPE_AUDIO
};

typedef void (*CLEAR_CALLBACK)(void *data);

typedef void (*BUFFERING_CALLBACK)(void *data, bool is_buffering);


template<class T>
class JSQueue {
public:

    JSQueue(QUEUE_TYPE queue_type, AVRational time_base, int64_t min_duration, int64_t max_duration,
            bool is_live);

    virtual  ~JSQueue();

    void set_clear_callback(CLEAR_CALLBACK callback, void *data);

    void set_buffering_callback(BUFFERING_CALLBACK callback, void *data);

    virtual void clear(bool is_keep_signal_element) = 0;

    int64_t get_num();

    virtual int64_t get_duration() = 0;

    virtual T get() = 0;

    JS_RET (JSQueue::*put)(T element);

    void abort_get();

    void clear_abort_get();

    void abort_put();

    void clear_abort_put();


    deque<T> m_dequeue0, *m_dequeue = &m_dequeue0;

    pthread_mutex_t m_mutex0, *m_mutex = &m_mutex0;
    pthread_mutexattr_t m_mutexattr0, *m_mutexattr = &m_mutexattr0;
    pthread_cond_t m_cond0, *m_cond = &m_cond0;

    volatile bool m_is_waiting_for_reach_min_duration = true;
    volatile bool m_is_waiting_for_insufficient_max_duration = false;
    volatile bool m_is_need_to_drop_until_i_frame = true;

    volatile bool m_is_aborted_get = false;
    volatile bool m_is_aborted_put = false;

    QUEUE_TYPE m_queue_type;
    AVRational m_time_base;
    int64_t m_min_duration = -1;
    int64_t m_max_duration = -1;
    bool m_is_live;

    int64_t m_buffer_duration = 0;

    int64_t m_num = 0;

    CLEAR_CALLBACK clear_callback = NULL;
    void *m_clear_callback_data = NULL;

    BUFFERING_CALLBACK buffering_callback = NULL;
    void *m_buffering_callback_data = NULL;
};


template<class T>
JSQueue<T>::JSQueue(QUEUE_TYPE queue_type, AVRational time_base, int64_t min_duration,
                    int64_t max_duration,
                    bool is_live) {

    pthread_mutexattr_settype(m_mutexattr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(m_mutex, m_mutexattr);
    pthread_cond_init(m_cond, NULL);

    m_queue_type = queue_type;
    m_time_base = time_base;
    m_min_duration = min_duration;
    m_max_duration = max_duration;
    m_is_live = is_live;
}


template<class T>
JSQueue<T>::~JSQueue() {
    pthread_mutex_destroy(m_mutex);
    pthread_mutexattr_destroy(m_mutexattr);
    pthread_cond_destroy(m_cond);
}

template<class T>
void JSQueue<T>::set_clear_callback(CLEAR_CALLBACK callback, void *data) {
    clear_callback = callback;
    m_clear_callback_data = data;
}

template<class T>
void JSQueue<T>::set_buffering_callback(BUFFERING_CALLBACK callback, void *data) {
    buffering_callback = callback;
    m_buffering_callback_data = data;
}


template<class T>
int64_t JSQueue<T>::get_num() {
    return m_num;
}

template<class T>
void JSQueue<T>::abort_get() {

    pthread_mutex_lock(m_mutex);
    m_is_aborted_get = true;
    if (m_is_waiting_for_reach_min_duration) {
        pthread_cond_signal(m_cond);
    }
    pthread_mutex_unlock(m_mutex);

}

template<class T>
void JSQueue<T>::clear_abort_get() {
    pthread_mutex_lock(m_mutex);
    m_is_aborted_get = false;
    pthread_mutex_unlock(m_mutex);
}

template<class T>
void JSQueue<T>::abort_put() {

    pthread_mutex_lock(m_mutex);
    m_is_aborted_put = true;
    if (m_is_waiting_for_insufficient_max_duration) {
        pthread_cond_signal(m_cond);
    }
    pthread_mutex_unlock(m_mutex);

}

template<class T>
void JSQueue<T>::clear_abort_put() {
    pthread_mutex_lock(m_mutex);
    m_is_aborted_put = false;
    pthread_mutex_unlock(m_mutex);
}


class JSPacketQueue : public JSQueue<AVPacket *> {
public:

    AVPacket m_flush_pkt0, *m_flush_pkt = &m_flush_pkt0;
    AVPacket m_eof_pkt0, *m_eof_pkt = &m_eof_pkt0;

    JSPacketQueue(QUEUE_TYPE queue_type, AVRational time_base, int64_t min_duration,
                  int64_t max_duration, bool is_live) : JSQueue<AVPacket *>(queue_type,
                                                                            time_base,
                                                                            min_duration,
                                                                            max_duration,
                                                                            is_live) {


        if (m_queue_type == QUEUE_TYPE_VIDEO) {

            if (m_is_live) {
                put = (JS_RET(JSQueue<AVPacket *>::*)(
                        AVPacket *)) &JSPacketQueue::put_live_video_packet;
            } else {
                put = (JS_RET(JSQueue<AVPacket *>::*)(
                        AVPacket *)) &JSPacketQueue::put_record_packet;
            }

        } else if (m_queue_type == QUEUE_TYPE_AUDIO) {
            if (m_is_live) {
                put = (JS_RET(JSQueue<AVPacket *>::*)(
                        AVPacket *)) &JSPacketQueue::put_live_audio_packet;
            } else {
                put = (JS_RET(JSQueue<AVPacket *>::*)(
                        AVPacket *)) &JSPacketQueue::put_record_packet;
            }
        }

        av_init_packet(m_flush_pkt);
        m_flush_pkt->size = 0;
        m_flush_pkt->data = (uint8_t *) m_flush_pkt;

        av_init_packet(m_eof_pkt);
        m_eof_pkt->size = 0;
        m_eof_pkt->data = (uint8_t *) m_eof_pkt;

    }

    ~JSPacketQueue() {
        clear(false);
    }


    AVPacket *get() {

        while (true) {
            pthread_mutex_lock(m_mutex);
            if (m_is_aborted_get) {
                goto aborted;
            }
            if (!m_is_waiting_for_reach_min_duration) {

                AVPacket *element = m_dequeue->front();
                m_dequeue->pop_front();

                m_num--;

                if (m_num == 0) {
                    m_is_waiting_for_reach_min_duration = true;
                }

                if (m_is_waiting_for_insufficient_max_duration) {
                    pthread_cond_signal(m_cond);
                    m_is_waiting_for_insufficient_max_duration = false;
                }

                pthread_mutex_unlock(m_mutex);

                return element;
            } else {

                if (buffering_callback) {
                    buffering_callback(m_buffering_callback_data, true);
                }
                pthread_cond_wait(m_cond, m_mutex);
                if (buffering_callback) {
                    buffering_callback(m_buffering_callback_data, false);
                }
                pthread_mutex_unlock(m_mutex);
            }
        }

        aborted:
        pthread_mutex_unlock(m_mutex);
        return NULL;

    }

    int64_t get_duration() {

        pthread_mutex_lock(m_mutex);
        AVPacket *last = NULL;
        AVPacket *first = NULL;
        int64_t duration = 0;

        if (m_num <= 1) {
            goto return0;
        }

        last = m_dequeue->back();
        if (last->size == 0) {
            deque<AVPacket *>::iterator it = m_dequeue->end();
            it--;
            last = *it--;
        }

        first = m_dequeue->front();
        if (first->size == 0) {
            deque<AVPacket *>::iterator it = m_dequeue->begin();
            first = *it++;
        }

        duration = (int64_t) ((last->dts - first->dts) * av_q2d(m_time_base) * 1000000);
        pthread_mutex_unlock(m_mutex);
        return duration;

        return0:
        pthread_mutex_unlock(m_mutex);
        return 0;
    }


    void clear(bool is_keep_signal_element) {

        pthread_mutex_lock(m_mutex);

        if (m_num == 0) {
            pthread_mutex_unlock(m_mutex);
            return;
        }

        m_num = 0;
        deque<AVPacket *>::iterator it;

        for (it = m_dequeue->begin();
             it != m_dequeue->end();) {

            if ((*it)->size == 0 && is_keep_signal_element) {
                it++;
                m_num++;
                continue;
            }

            av_packet_free(&*it);
            it = m_dequeue->erase(it);
        }

        if (m_queue_type == QUEUE_TYPE_VIDEO) {
            m_is_need_to_drop_until_i_frame = true;
        }

        if (m_num == 0) {
            m_is_waiting_for_reach_min_duration = true;
        }

        if (m_is_waiting_for_insufficient_max_duration) {
            pthread_cond_signal(m_cond);
            m_is_waiting_for_insufficient_max_duration = false;
        }

        if (clear_callback) {
            clear_callback(m_clear_callback_data);
        }
        pthread_mutex_unlock(m_mutex);
    }


    JS_RET put_live_video_packet(AVPacket *avpkt) {

        pthread_mutex_lock(m_mutex);

        AVPacket *src = av_packet_alloc();
        if (src == NULL) {
            LOGE("%s can't av_packet_alloc", __func__);
            pthread_mutex_unlock(m_mutex);
            return JS_ERR;
        }

        *src = *avpkt;

        bool is_signal_avpkt = src->size == 0;

        if (m_is_need_to_drop_until_i_frame && !is_signal_avpkt) {
            if (src->flags & AV_PKT_FLAG_KEY) {
                m_is_need_to_drop_until_i_frame = false;
            } else {
                av_packet_free(&src);
                LOGD("%s *drop* drop a non-key frame until key video packet.", __func__);
                pthread_mutex_unlock(m_mutex);
                return JS_OK;
            }
        }

        bool is_eof_avpkt = src->data == m_eof_pkt->data;
        if (is_eof_avpkt) {
            src->data = NULL;
        }

        m_dequeue->push_back(src);
        m_num++;

        int64_t duration = get_duration();

        if (duration >= m_max_duration) {

            if (is_eof_avpkt) {
                clear(true);
            } else {
                clear(false);
                if (JS_OK != put_flush_avpkt()) {
                    LOGE("%s can't put video flush packet.", __func__);
                    pthread_mutex_unlock(m_mutex);
                    return JS_ERR;
                }
            }
            if (m_is_waiting_for_reach_min_duration) {
                pthread_cond_signal(m_cond);
                m_is_waiting_for_reach_min_duration = false;
            }

        } else if (m_is_waiting_for_reach_min_duration && (
                duration >= m_min_duration || is_signal_avpkt)) {
            pthread_cond_signal(m_cond);
            m_is_waiting_for_reach_min_duration = false;
        }
        pthread_mutex_unlock(m_mutex);
        return JS_OK;
    }


    JS_RET put_live_audio_packet(AVPacket *avpkt) {

        pthread_mutex_lock(m_mutex);

        AVPacket *src = av_packet_alloc();
        if (src == NULL) {
            LOGE("%s can't av_packet_alloc", __func__);
            pthread_mutex_unlock(m_mutex);
            return JS_ERR;
        }

        *src = *avpkt;

        m_dequeue->push_back(src);
        m_num++;

        int64_t duration = get_duration();

        bool is_signal_avpkt = src->size == 0;
        bool is_eof_avpkt = src->data == m_eof_pkt->data;
        if (is_eof_avpkt) {
            src->data = NULL;
        }

        if (duration >= m_max_duration) {
            if (is_eof_avpkt) {
                clear(true);
            } else {
                clear(false);
                if (JS_OK != put_flush_avpkt()) {
                    LOGE("%s can't put video flush packet.", __func__);
                    pthread_mutex_unlock(m_mutex);
                    return JS_ERR;
                }
            }
            if (m_is_waiting_for_reach_min_duration) {
                pthread_cond_signal(m_cond);
                m_is_waiting_for_reach_min_duration = false;
            }
        } else if (m_is_waiting_for_reach_min_duration &&
                   (duration >= m_min_duration || is_signal_avpkt)) {
            pthread_cond_signal(m_cond);
            m_is_waiting_for_reach_min_duration = false;
        }

        pthread_mutex_unlock(m_mutex);
        return JS_OK;
    }

    JS_RET put_record_packet(AVPacket *avpkt) {

        pthread_mutex_lock(m_mutex);

        AVPacket *src = av_packet_alloc();
        if (src == NULL) {
            LOGE("%s can't av_packet_alloc", __func__);
            pthread_mutex_unlock(m_mutex);
            return JS_ERR;
        }

        *src = *avpkt;

        m_dequeue->push_back(src);
        m_num++;

        int64_t duration = get_duration();

        bool is_signal_avpkt = src->size == 0;
        bool is_eof_avpkt = src->data == m_eof_pkt->data;

        if (is_eof_avpkt) {
            src->data = NULL;
        }

        if (m_is_waiting_for_reach_min_duration &&
            (duration >= m_min_duration || is_signal_avpkt)) {
            pthread_cond_signal(m_cond);
            m_is_waiting_for_reach_min_duration = false;
        }

        if (duration >= m_max_duration) {

            if (m_is_aborted_put) {
                pthread_mutex_unlock(m_mutex);
                return JS_OK;
            }
            m_is_waiting_for_insufficient_max_duration = true;
            pthread_cond_wait(m_cond, m_mutex);

        }

        pthread_mutex_unlock(m_mutex);
        return JS_OK;
    }

    JS_RET put_eof_avpkt() {
        return (this->*put)(m_eof_pkt);
    }

    JS_RET put_flush_avpkt() {
        return (this->*put)(m_flush_pkt);
    }

};


class JSFrameQueue : public JSQueue<AVFrame *> {
public:
    JSFrameQueue(QUEUE_TYPE queue_type, AVRational time_base, int64_t min_duration,
                 int64_t max_duration, bool is_live) : JSQueue<AVFrame *>(queue_type,
                                                                          time_base,
                                                                          min_duration,
                                                                          max_duration,
                                                                          is_live) {

        if (m_is_live) {
            put = (JS_RET(JSQueue<AVFrame *>::*)(AVFrame *)) &JSFrameQueue::put_live_frame;
        } else {
            put = (JS_RET(JSQueue<AVFrame *>::*)(AVFrame *)) &JSFrameQueue::put_record_frame;
        }

    }

    ~JSFrameQueue() {
        clear(false);
    }

    AVFrame *get() {

        while (true) {
            pthread_mutex_lock(m_mutex);
            if (m_is_aborted_get) {
                goto aborted;
            }
            if (!m_is_waiting_for_reach_min_duration) {

                AVFrame *element = m_dequeue->front();
                m_dequeue->pop_front();

                m_num--;

                if (m_num == 0) {
                    m_is_waiting_for_reach_min_duration = true;
                }

                if (m_is_waiting_for_insufficient_max_duration) {
                    pthread_cond_signal(m_cond);
                    m_is_waiting_for_insufficient_max_duration = false;
                }

                pthread_mutex_unlock(m_mutex);

                return element;
            } else {

                if (buffering_callback) {
                    buffering_callback(m_buffering_callback_data, true);
                }
                pthread_cond_wait(m_cond, m_mutex);
                if (buffering_callback) {
                    buffering_callback(m_buffering_callback_data, false);
                }
                pthread_mutex_unlock(m_mutex);
            }
        }

        aborted:
        pthread_mutex_unlock(m_mutex);
        return NULL;

    }

    int64_t get_duration() {

        pthread_mutex_lock(m_mutex);

        AVFrame *last = NULL;
        AVFrame *first = NULL;
        int64_t duration = 0;

        if (m_num <= 1) {
            goto return0;
        }

        last = m_dequeue->back();
        first = m_dequeue->front();

        duration = (int64_t) ((last->pts - first->pts) * av_q2d(m_time_base) * 1000000);
        pthread_mutex_unlock(m_mutex);
        return duration;

        return0:
        pthread_mutex_unlock(m_mutex);
        return 0;
    }


    void clear(bool is_keep_signal_element) {

        pthread_mutex_lock(m_mutex);

        if (m_num == 0) {
            pthread_mutex_unlock(m_mutex);
            return;
        }

        m_num = 0;
        deque<AVFrame *>::iterator it;

        for (it = m_dequeue->begin();
             it != m_dequeue->end();) {

            av_frame_free(&*it);
            it++;
        }
        m_dequeue->clear();

        m_is_waiting_for_reach_min_duration = true;

        if (m_is_waiting_for_insufficient_max_duration) {
            pthread_cond_signal(m_cond);
            m_is_waiting_for_insufficient_max_duration = false;
        }

        if (clear_callback) {
            clear_callback(m_clear_callback_data);
        }
        pthread_mutex_unlock(m_mutex);
    }


    JS_RET put_live_frame(AVFrame *frame) {

        pthread_mutex_lock(m_mutex);

        AVFrame *src = av_frame_alloc();
        if (src == NULL) {
            LOGE("%s unable to allocate an AVFrame!", __func__);
            av_frame_unref(frame);
            pthread_mutex_unlock(m_mutex);
            return JS_ERR;
        }

        av_frame_move_ref(src, frame);

        m_dequeue->push_back(src);
        m_num++;

        int64_t duration = get_duration();

        if (duration >= m_max_duration) {
            clear(false);
        } else if (m_is_waiting_for_reach_min_duration && duration >= m_min_duration) {
            pthread_cond_signal(m_cond);
            m_is_waiting_for_reach_min_duration = false;
        }

        pthread_mutex_unlock(m_mutex);
        return JS_OK;
    }


    JS_RET put_record_frame(AVFrame *frame) {

        pthread_mutex_lock(m_mutex);

        AVFrame *src = av_frame_alloc();
        if (src == NULL) {
            LOGE("%s unable to allocate an AVFrame!", __func__);
            pthread_mutex_unlock(m_mutex);
            return JS_ERR;
        }

        av_frame_move_ref(src, frame);

        m_dequeue->push_back(src);
        m_num++;

        int64_t duration = get_duration();

        if (m_is_waiting_for_reach_min_duration && duration >= m_min_duration) {
            pthread_cond_signal(m_cond);
            m_is_waiting_for_reach_min_duration = false;
        }

        if (duration >= m_max_duration) {

            if (m_is_aborted_put) {
                pthread_mutex_unlock(m_mutex);
                return JS_OK;
            }
            m_is_waiting_for_insufficient_max_duration = true;
            pthread_cond_wait(m_cond, m_mutex);
        }

        pthread_mutex_unlock(m_mutex);

        return JS_OK;
    }

};

#endif //JS_QUEUE_H