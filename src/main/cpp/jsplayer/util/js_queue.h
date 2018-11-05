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

enum CLEAR_MODE {
    CLEAR_ALL = 0,
    CLEAR_KEEP_KEY_FRAME
};

enum QUEUE_TYPE {
    QUEUE_TYPE_VIDEO = 0,
    QUEUE_TYPE_AUDIO
};

template<class T>
class JSQueue {
public:

    JSQueue(QUEUE_TYPE queue_type, AVRational time_base, int64_t min_duration, int64_t max_duration,
            bool is_live);

    ~JSQueue();

    T get();

    T get_last();

    int64_t get_num();

//    void put(T src);

//    int64_t get_duration();

//    void clear(CLEAR_MODE mode);

    int64_t (*get_duration)(void *queue_);

    void (*clear)(void *queue_, CLEAR_MODE mode);

    void (*put)(void *queue_, void *src_);

    void abort_get();

    void clear_abort_get();

    void abort_put();

    void clear_abort_put();

    deque<T> m_dequeue;

    pthread_mutex_t m_mutex;
    pthread_mutexattr_t m_mutexattr;
    pthread_cond_t m_cond;
    volatile bool m_is_waiting_for_reach_min_duration = false;
    volatile bool m_is_waiting_for_insufficient_max_duration = false;
    volatile bool m_is_need_to_drop_until_i_frame = false;

    volatile bool is_reached_the_min_duration = false;
    volatile bool m_is_aborted_get = false;
    volatile bool m_is_aborted_put = false;

    QUEUE_TYPE m_queue_type;
    AVRational m_time_base;
    int64_t m_min_duration = -1;
    int64_t m_max_duration = -1;
    bool m_is_live;

};


static int64_t get_cached_duration(void *queue_);

static int64_t get_decoded_duration(void *queue_);

static void clear_packet(void *queue_, CLEAR_MODE mode);

static void clear_frame(void *queue_, CLEAR_MODE mode);

static void put_live_video_packet(void *queue_, void *src_);

static void put_live_video_frame(void *queue_, void *src_);

static void put_live_audio_packet(void *queue_, void *src_);

static void put_live_audio_frame(void *queue_, void *src_);

static void put_record_packet(void *queue_, void *src_);

static void put_record_frame(void *queue_, void *src_);


template<class T>
JSQueue<T>::JSQueue(QUEUE_TYPE queue_type, AVRational time_base, int64_t min_duration,
                    int64_t max_duration,
                    bool is_live) {

    pthread_mutexattr_settype(&m_mutexattr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&m_mutex, &m_mutexattr);
    pthread_cond_init(&m_cond, NULL);

    m_queue_type = queue_type;
    m_time_base = time_base;
    m_min_duration = min_duration;
    m_max_duration = max_duration;
    m_is_live = is_live;

    if (typeid(T) == typeid(AVPacket *)) {
        get_duration = get_cached_duration;
        clear = clear_packet;

        if (m_queue_type == QUEUE_TYPE_VIDEO) {

            if (m_is_live) {
                put = put_live_video_packet;
            } else {
                put = put_record_packet;
            }

        } else if (m_queue_type == QUEUE_TYPE_AUDIO) {
            if (m_is_live) {
                put = put_live_audio_packet;
            } else {
                put = put_record_packet;
            }
        }
    } else if (typeid(T) == typeid(AVFrame *)) {
        get_duration = get_decoded_duration;
        clear = clear_frame;

        if (m_queue_type == QUEUE_TYPE_VIDEO) {

            if (m_is_live) {
                put = put_live_video_frame;
            } else {
                put = put_record_frame;
            }

        } else if (m_queue_type == QUEUE_TYPE_AUDIO) {
            if (m_is_live) {
                put = put_live_audio_frame;
            } else {
                put = put_record_frame;
            }
        }
    }
}


template<class T>
JSQueue<T>::~JSQueue() {
    clear(this, CLEAR_ALL);
    pthread_mutex_destroy(&m_mutex);
    pthread_mutexattr_destroy(&m_mutexattr);
    pthread_cond_destroy(&m_cond);
}

template<class T>
T JSQueue<T>::get() {

    while (true) {
        pthread_mutex_lock(&m_mutex);
        if (is_reached_the_min_duration) {
            if (m_is_aborted_get) {
                goto aborted;
            }
            T tmp = m_dequeue.front();
            m_dequeue.pop_front();

            if (m_dequeue.empty()) {
                is_reached_the_min_duration = false;
            } else if (m_is_waiting_for_insufficient_max_duration) {
                pthread_cond_signal(&m_cond);
            }
            pthread_mutex_unlock(&m_mutex);
            return tmp;
        } else {
            if (m_is_aborted_get) {
                goto aborted;
            }
            m_is_waiting_for_reach_min_duration = true;
            pthread_cond_wait(&m_cond, &m_mutex);
            m_is_waiting_for_reach_min_duration = false;
            if (m_is_aborted_get) {
                goto aborted;
            }
            pthread_mutex_unlock(&m_mutex);
        }
    }

    aborted:
    pthread_mutex_unlock(&m_mutex);
    return NULL;

}


template<class T>
T JSQueue<T>::get_last() {
    pthread_mutex_lock(&m_mutex);
    T last;
    if (m_dequeue.empty()) {
        last = NULL;
    } else {
        last = m_dequeue.back();
    }
    pthread_mutex_unlock(&m_mutex);
    return last;
}

template<class T>
int64_t JSQueue<T>::get_num() {
    pthread_mutex_lock(&m_mutex);
    int64_t size = m_dequeue.size();
    pthread_mutex_unlock(&m_mutex);
    return size;
}

template<class T>
void JSQueue<T>::abort_get() {

    pthread_mutex_lock(&m_mutex);
    m_is_aborted_get = true;
    if (m_is_waiting_for_reach_min_duration) {
        pthread_cond_signal(&m_cond);
    }
    pthread_mutex_unlock(&m_mutex);

}

template<class T>
void JSQueue<T>::clear_abort_get() {
    pthread_mutex_lock(&m_mutex);
    m_is_aborted_get = false;
    pthread_mutex_unlock(&m_mutex);
}

template<class T>
void JSQueue<T>::abort_put() {

    pthread_mutex_lock(&m_mutex);
    m_is_aborted_put = true;
    if (m_is_waiting_for_insufficient_max_duration) {
        pthread_cond_signal(&m_cond);
    }
    pthread_mutex_unlock(&m_mutex);

}

template<class T>
void JSQueue<T>::clear_abort_put() {
    pthread_mutex_lock(&m_mutex);
    m_is_aborted_put = false;
    pthread_mutex_unlock(&m_mutex);
}


int64_t get_cached_duration(void *queue_) {
    JSQueue<AVPacket *> *queue = (JSQueue<AVPacket *> *) queue_;
    pthread_mutex_t *mutex = &queue->m_mutex;

    pthread_mutex_lock(mutex);
    deque<AVPacket *> *dequeue = &queue->m_dequeue;
    AVPacket *last;
    AVPacket *first;
    int64_t duration;

    if (dequeue->empty()) {
        goto return0;
    }

    last = dequeue->back();
    first = dequeue->front();
    if (last != NULL && first != NULL) {
        duration = av_rescale_q(last->dts - first->dts, queue->m_time_base,
                                AV_TIME_BASE_Q);
        pthread_mutex_unlock(mutex);
        return duration;
    } else {
        goto return0;
    }

    return0:
    pthread_mutex_unlock(mutex);
    return 0;
}

int64_t get_decoded_duration(void *queue_) {
    JSQueue<AVFrame *> *queue = (JSQueue<AVFrame *> *) queue_;
    pthread_mutex_t *mutex = &queue->m_mutex;


    pthread_mutex_lock(mutex);
    deque<AVFrame *> *dequeue = &queue->m_dequeue;

    AVFrame *last;
    AVFrame *first;
    int64_t duration;

    if (dequeue->empty()) {
        goto return0;
    }


    last = dequeue->back();
    first = dequeue->front();
    if (last != NULL && first != NULL) {
        duration = av_rescale_q(last->pts - first->pts, queue->m_time_base,
                                AV_TIME_BASE_Q);
        pthread_mutex_unlock(mutex);
        return duration;
    } else {
        goto return0;
    }


    return0:
    pthread_mutex_unlock(mutex);
    return 0;
}


void clear_packet(void *queue_, CLEAR_MODE mode) {
    JSQueue<AVPacket *> *queue = (JSQueue<AVPacket *> *) queue_;
    pthread_mutex_t *mutex = &queue->m_mutex;

    pthread_mutex_lock(mutex);

    deque<AVPacket *> *dequeue = &queue->m_dequeue;

    if (dequeue->empty()) {
        pthread_mutex_unlock(mutex);
        return;
    }
    deque<AVPacket *>::iterator it;

    if (CLEAR_KEEP_KEY_FRAME == mode) {
        for (it = dequeue->begin();
             it != dequeue->end();) {
            if ((*it)->flags & AV_PKT_FLAG_KEY) {
                LOGD("keep key AVPacket");
                it++;
                continue;
            }
            av_packet_free(&*it);
            it = dequeue->erase(it);
        }
    } else {
        for (it = dequeue->begin();
             it != dequeue->end();) {
            av_packet_free(&*it);
            it = dequeue->erase(it);
        }
    }

    if (dequeue->empty()) {
        queue->is_reached_the_min_duration = false;
    } else if (queue->m_is_waiting_for_insufficient_max_duration) {
        pthread_cond_signal(&queue->m_cond);
    }


    if (queue->m_queue_type == QUEUE_TYPE_VIDEO) {
        queue->m_is_need_to_drop_until_i_frame = true;
    }
    pthread_mutex_unlock(mutex);
}

void clear_frame(void *queue_, CLEAR_MODE mode) {
    JSQueue<AVFrame *> *queue = (JSQueue<AVFrame *> *) queue_;
    pthread_mutex_t *mutex = &queue->m_mutex;

    pthread_mutex_lock(mutex);

    deque<AVFrame *> *dequeue = &queue->m_dequeue;

    if (dequeue->empty()) {
        pthread_mutex_unlock(mutex);
        return;
    }

    deque<AVFrame *>::iterator it;

    if (CLEAR_KEEP_KEY_FRAME == mode) {

        for (it = dequeue->begin();
             it != dequeue->end();) {

            if ((*it)->key_frame) {
                LOGD("keep key AVFrame");
                it++;
                continue;
            }

            if ((*it)->opaque) {
                av_free((*it)->opaque);
            }
            av_frame_free(&*it);
            it = dequeue->erase(it);
        }
    } else {
        for (it = dequeue->begin();
             it != dequeue->end();) {

            if ((*it)->opaque) {//todo judge is only mediacodec use
                av_free((*it)->opaque);
            }
            av_frame_free(&*it);
            it = dequeue->erase(it);
        }
    }
    if (dequeue->empty()) {
        queue->is_reached_the_min_duration = false;
    } else if (queue->m_is_waiting_for_insufficient_max_duration) {
        pthread_cond_signal(&queue->m_cond);
    }
    pthread_mutex_unlock(mutex);
}

void put_live_video_packet(void *queue_, void *src_) {
    JSQueue<AVPacket *> *queue = (JSQueue<AVPacket *> *) queue_;
    pthread_mutex_t *mutex = &queue->m_mutex;

    pthread_mutex_lock(mutex);
    LOGD("put_live_video_packet");

    deque<AVPacket *> *dequeue = &queue->m_dequeue;
    AVPacket *src = (AVPacket *) src_;

    if (queue->m_is_need_to_drop_until_i_frame) {
        if (src->flags & AV_PKT_FLAG_KEY) {
            queue->m_is_need_to_drop_until_i_frame = false;
            dequeue->push_back(src);
            LOGW("put_live_video_packet put a key frame.");
        } else {
            av_packet_free(&src);
            LOGW("put_live_video_packet drop a non key frame."); //fixme to many.
        }
    } else {
        //todo 尝试丢掉两个非连续B帧（a,b)之前的所有帧。除b帧往前到前一个B帧之间的帧。动态监测算法。


        int64_t duration = queue->get_duration(queue);
        if (duration > queue->m_max_duration) {
            LOGW("put_live_video_packet drop video packet ,duration=%"
                 PRId64
                 ",num=%d",
                 duration, queue->get_num());
            //last packet isn't key frame,need drop until i frame.
            if (!(queue->get_last()->flags & AV_PKT_FLAG_KEY)) {
                queue->m_is_need_to_drop_until_i_frame = true;
            }
            queue->clear(queue, CLEAR_KEEP_KEY_FRAME);

            duration = queue->get_duration(queue);
            if (duration > queue->m_max_duration) {
                queue->clear(queue, CLEAR_ALL);
                queue->m_is_need_to_drop_until_i_frame = true;
            }

            if (queue->m_is_need_to_drop_until_i_frame) {
                av_packet_free(&src);
            } else {
                dequeue->push_back(src);
            }
        } else if (duration >= queue->m_min_duration) {
            queue->is_reached_the_min_duration = true;

            if (queue->m_is_waiting_for_reach_min_duration) {
                pthread_cond_signal(&queue->m_cond);
            }
            dequeue->push_back(src);
        } else {
            dequeue->push_back(src);
        }


    }
    pthread_mutex_unlock(mutex);
}

void put_live_video_frame(void *queue_, void *src_) {
    JSQueue<AVFrame *> *queue = (JSQueue<AVFrame *> *) queue_;
    pthread_mutex_t *mutex = &queue->m_mutex;

    pthread_mutex_lock(mutex);

    deque<AVFrame *> *dequeue = &queue->m_dequeue;
    AVFrame *src = (AVFrame *) src_;


    //todo 尝试丢掉两个非连续B帧（a,b)之前的所有帧。除b帧往前到前一个B帧之间的帧。动态监测算法。
    int64_t duration = queue->get_duration(queue);
    if (duration > queue->m_max_duration) {
        LOGW("drop video frame ,duration=%"
             PRId64
             ", num=%d",
             duration, queue->get_num());
        queue->clear(queue, CLEAR_KEEP_KEY_FRAME);
        duration = queue->get_duration(queue);
        if (duration > queue->m_max_duration) {
            queue->clear(queue, CLEAR_ALL);
        }
    } else if (duration >= queue->m_min_duration) {

        queue->is_reached_the_min_duration = true;

        if (queue->m_is_waiting_for_reach_min_duration) {
            pthread_cond_signal(&queue->m_cond);
        }

    }

    dequeue->push_back(src);

    pthread_mutex_unlock(mutex);
}

void put_live_audio_packet(void *queue_, void *src_) {
    JSQueue<AVPacket *> *queue = (JSQueue<AVPacket *> *) queue_;
    pthread_mutex_t *mutex = &queue->m_mutex;

    pthread_mutex_lock(mutex);

    deque<AVPacket *> *dequeue = &queue->m_dequeue;
    AVPacket *src = (AVPacket *) src_;

    //todo 尝试丢掉两个非连续B帧（a,b)之前的所有帧。除b帧往前到前一个B帧之间的帧。动态监测算法。
    int64_t duration = queue->get_duration(queue);
    if (duration > queue->m_max_duration) {
        LOGW("drop audio packet ,duration=%"
             PRId64
             ", num=%d",
             duration, queue->get_num());
        queue->clear(queue, CLEAR_ALL);
    } else if (duration >= queue->m_min_duration) {

        queue->is_reached_the_min_duration = true;

        if (queue->m_is_waiting_for_reach_min_duration) {
            pthread_cond_signal(&queue->m_cond);
        }

    }

    dequeue->push_back(src);

    pthread_mutex_unlock(mutex);
}

void put_live_audio_frame(void *queue_, void *src_) {
    JSQueue<AVFrame *> *queue = (JSQueue<AVFrame *> *) queue_;
    pthread_mutex_t *mutex = &queue->m_mutex;

    pthread_mutex_lock(mutex);

    deque<AVFrame *> *dequeue = &queue->m_dequeue;
    AVFrame *src = (AVFrame *) src_;
    //todo 尝试丢掉两个非连续B帧（a,b)之前的所有帧。除b帧往前到前一个B帧之间的帧。动态监测算法。
    int64_t duration = queue->get_duration(queue);
    if (duration > queue->m_max_duration) {
        LOGW("drop audio frame ,duration=%"
             PRId64
             ", num=%d",
             duration, queue->get_num());
        queue->clear(queue, CLEAR_ALL);
    } else if (duration >= queue->m_min_duration) {

        queue->is_reached_the_min_duration = true;

        if (queue->m_is_waiting_for_reach_min_duration) {
            pthread_cond_signal(&queue->m_cond);
        }

    }

    dequeue->push_back(src);

    pthread_mutex_unlock(mutex);
}

void put_record_packet(void *queue_, void *src_) {
    JSQueue<AVPacket *> *queue = (JSQueue<AVPacket *> *) queue_;
    pthread_mutex_t *mutex = &queue->m_mutex;

    pthread_mutex_lock(mutex);

    deque<AVPacket *> *dequeue = &queue->m_dequeue;
    AVPacket *src = (AVPacket *) src_;

    dequeue->push_back(src);

    if (queue->m_is_aborted_put) {
        pthread_mutex_unlock(mutex);
        return;
    }

    int64_t duration = queue->get_duration(queue);

    if (duration >= queue->m_max_duration) {

        queue->m_is_waiting_for_insufficient_max_duration = true;
        pthread_cond_wait(&queue->m_cond, mutex);
        queue->m_is_waiting_for_insufficient_max_duration = false;

    } else if (duration >= queue->m_min_duration) {

        queue->is_reached_the_min_duration = true;

        if (queue->m_is_waiting_for_reach_min_duration) {
            pthread_cond_signal(&queue->m_cond);
        }

    }

    pthread_mutex_unlock(mutex);
}

void put_record_frame(void *queue_, void *src_) {
    JSQueue<AVFrame *> *queue = (JSQueue<AVFrame *> *) queue_;
    pthread_mutex_t *mutex = &queue->m_mutex;

    pthread_mutex_lock(mutex);

    deque<AVFrame *> *dequeue = &queue->m_dequeue;
    AVFrame *src = (AVFrame *) src_;


    dequeue->push_back(src);


    if (queue->m_is_aborted_put) {
        pthread_mutex_unlock(mutex);
        return;
    }

    int64_t duration = queue->get_duration(queue);

    if (duration >= queue->m_max_duration) {

        queue->m_is_waiting_for_insufficient_max_duration = true;
        pthread_cond_wait(&queue->m_cond, mutex);
        queue->m_is_waiting_for_insufficient_max_duration = false;

    } else if (duration >= queue->m_min_duration) {

        queue->is_reached_the_min_duration = true;

        if (queue->m_is_waiting_for_reach_min_duration) {
            pthread_cond_signal(&queue->m_cond);
        }

    }

    pthread_mutex_unlock(mutex);
}


#endif //JS_QUEUE_H
