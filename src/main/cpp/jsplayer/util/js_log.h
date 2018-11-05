#ifndef JS_LOG_H
#define JS_LOG_H

#include "stdarg.h"

extern int LOGGABLE;
extern char *LOG_FILE_PATH;

void write_log(int prio, const char *tag, const char *text);

void LOGV(const char *fmt, ...);

void LOGD(const char *fmt, ...);

void LOGI(const char *fmt, ...);

void LOGW(const char *fmt, ...);

void LOGE(const char *fmt, ...);

extern void ffp_log_callback_report(void *ptr, int level, const char *fmt, va_list vl);

#endif //JS_LOG_H


