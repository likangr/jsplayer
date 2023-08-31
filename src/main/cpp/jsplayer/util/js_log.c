#include "js_log.h"
#include "js_jni.h"
#include <time.h>
#include <stdio.h>
#include <pthread.h>

static bool loggable = false;
static bool is_write_log_to_file = false;
static char log_file_save_path[256] = {0};
static pthread_mutex_t log_mutex0, *log_mutex = &log_mutex0;

#define LOG_BUF_SIZE      1024
#define LOG_TIME_SIZE     30
#define LOG_TAG          "jsplayer"

void init_logger() {
    pthread_mutex_init(log_mutex, NULL);
}

void deinit_logger() {
    pthread_mutex_destroy(log_mutex);
}

void set_loggable_(bool loggable_) {
    loggable = loggable_;
}

bool get_loggable_() {
    return loggable;
}

void set_is_write_log_to_file_(bool is_write_log_to_file_) {
    is_write_log_to_file = is_write_log_to_file_;
}

bool get_is_write_log_to_file_() {
    return is_write_log_to_file;
}

void set_log_file_save_path_(char *log_file_save_path_) {
    memset(log_file_save_path, sizeof(log_file_save_path), sizeof(char));
    strcpy(log_file_save_path, log_file_save_path_);
}

void get_log_file_save_path_(char *log_file_save_path_) {
    strcpy(log_file_save_path_, log_file_save_path);
}

void printf_log(int prio, const char *fmt, ...) {
    if (!loggable) {
        return;
    }

    va_list ap;
    char log_buf[LOG_BUF_SIZE];
    va_start(ap, fmt);
    vsnprintf(log_buf, LOG_BUF_SIZE, fmt, ap);
    va_end(ap);
    __android_log_write(prio, LOG_TAG, log_buf);

    if (!is_write_log_to_file) {
        return;
    }
    pthread_mutex_lock(log_mutex);
    FILE *fp = fopen(log_file_save_path, "at");
    if (!fp) {
        goto end;
    }

    time_t now_time;
    time(&now_time);
    struct tm *local_time = localtime(&now_time);
    char log_time[LOG_TIME_SIZE] = {0};
    strftime(log_time, LOG_TIME_SIZE, "[%Y-%m-%d %H:%M:%S]", local_time);
    fprintf(fp, "%s LogPriority=%d %s %s\r\n", log_time, prio, LOG_TAG, log_buf);
    fclose(fp);

    end:
    pthread_mutex_unlock(log_mutex);
}