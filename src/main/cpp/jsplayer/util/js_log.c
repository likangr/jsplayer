#include "util/js_log.h"
#include "libavutil/log.h"
#include <android/log.h>
#include <time.h>

#define LOG_BUF_SIZE     1024
#define LOG_TAG          "jsplayer"

int LOGGABLE = 0;
char *LOG_FILE_PATH = NULL;

void write_log(int prio, const char *tag, const char *text) {

    __android_log_write(prio, tag, text);

    time_t now_time;
    time(&now_time);
    struct tm *local_time = localtime(&now_time);
    char log_time[30] = {'\0'};
    strftime(log_time, 30, "[%Y-%m-%d %H:%M:%S] ", local_time);

    FILE *fp;
    fp = fopen(LOG_FILE_PATH, "at");
    if (!fp) {
        __android_log_write(ANDROID_LOG_ERROR, tag, "can't open js_log.txt");
        return;
    }

    fprintf(fp, "js_log ");
    fprintf(fp, "%s", log_time);
    fprintf(fp, "%s\r\n", text);
    fclose(fp);
}

void LOGV(const char *fmt, ...) {
    if (!LOGGABLE)
        return;

    va_list ap;
    char log_buf[LOG_BUF_SIZE];
    va_start(ap, fmt);
    vsnprintf(log_buf, LOG_BUF_SIZE, fmt, ap);
    va_end(ap);

    write_log(ANDROID_LOG_VERBOSE, LOG_TAG, log_buf);
}

void LOGD(const char *fmt, ...) {
    if (!LOGGABLE)
        return;

    va_list ap;
    char log_buf[LOG_BUF_SIZE];
    va_start(ap, fmt);
    vsnprintf(log_buf, LOG_BUF_SIZE, fmt, ap);
    va_end(ap);

    write_log(ANDROID_LOG_DEBUG, LOG_TAG, log_buf);
}

void LOGI(const char *fmt, ...) {
    if (!LOGGABLE)
        return;

    va_list ap;
    char log_buf[LOG_BUF_SIZE];
    va_start(ap, fmt);
    vsnprintf(log_buf, LOG_BUF_SIZE, fmt, ap);
    va_end(ap);

    write_log(ANDROID_LOG_INFO, LOG_TAG, log_buf);
}

void LOGW(const char *fmt, ...) {
    if (!LOGGABLE)
        return;

    va_list ap;
    char log_buf[LOG_BUF_SIZE];
    va_start(ap, fmt);
    vsnprintf(log_buf, LOG_BUF_SIZE, fmt, ap);
    va_end(ap);

    write_log(ANDROID_LOG_WARN, LOG_TAG, log_buf);
}

void LOGE(const char *fmt, ...) {
    if (!LOGGABLE)
        return;

    va_list ap;
    char log_buf[LOG_BUF_SIZE];
    va_start(ap, fmt);
    vsnprintf(log_buf, LOG_BUF_SIZE, fmt, ap);
    va_end(ap);

    write_log(ANDROID_LOG_ERROR, LOG_TAG, log_buf);
}


void ffp_log_callback_report(void *ptr, int level, const char *fmt, va_list vl) {
    if (!LOGGABLE)
        return;
    int ffplv = ANDROID_LOG_VERBOSE;
    if (level <= AV_LOG_ERROR)
        ffplv = ANDROID_LOG_ERROR;
    else if (level <= AV_LOG_WARNING)
        ffplv = ANDROID_LOG_WARN;
    else if (level <= AV_LOG_INFO)
        ffplv = ANDROID_LOG_INFO;
    else if (level <= AV_LOG_VERBOSE)
        ffplv = ANDROID_LOG_VERBOSE;
    else
        ffplv = ANDROID_LOG_DEBUG;


    va_list vl2;
    char line[1024];
    static int print_prefix = 1;

    va_copy(vl2, vl);
    av_log_format_line(ptr, level, fmt, vl2, line, sizeof(line), &print_prefix);
    va_end(vl2);

    write_log(ffplv, LOG_TAG, line);
}
