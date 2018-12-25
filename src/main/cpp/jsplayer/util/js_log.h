#ifndef JS_LOG_H
#define JS_LOG_H

#include <android/log.h>
#include <stdbool.h>
#include <jni.h>

extern void init_logger();

extern void deinit_logger();

extern void set_loggable_(bool loggable_);

extern bool get_loggable_();

extern void set_is_write_log_to_file_(bool is_write_log_to_file_);

extern bool get_is_write_log_to_file_();

extern void set_log_file_save_path_(char *log_file_save_path_);

extern void get_log_file_save_path_(char *log_file_save_path_);

extern void printf_log(int prio, const char *fmt, ...);

#define LOGV(fmt, ...)  printf_log(ANDROID_LOG_VERBOSE, fmt, ##__VA_ARGS__)
#define LOGD(fmt, ...)  printf_log(ANDROID_LOG_DEBUG, fmt, ##__VA_ARGS__)
#define LOGI(fmt, ...)  printf_log(ANDROID_LOG_INFO, fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...)  printf_log(ANDROID_LOG_WARN, fmt, ##__VA_ARGS__)
#define LOGE(fmt, ...)  printf_log(ANDROID_LOG_ERROR, fmt, ##__VA_ARGS__)

#endif //JS_LOG_H


