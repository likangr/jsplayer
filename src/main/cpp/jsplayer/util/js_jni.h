#ifndef JS_JNI_H
#define JS_JNI_H

#include <jni.h>

extern int js_get_sdk_version();

extern int __JS_ANDROID_SDK_VERSION__;

int js_jni_set_java_vm(void *vm, void *log_ctx);

/*
 * Attach permanently a JNI environment to the current thread and retrieve it.
 *
 * If successfully attached, the JNI environment will automatically be detached
 * at thread destruction.
 *
 * @param attached pointer to an integer that will be set to 1 if the
 * environment has been attached to the current thread or 0 if it is
 * already attached.
 * @param log_ctx context used for logging, can be NULL
 * @return the JNI environment on success, NULL otherwise
 */
JNIEnv *js_jni_get_env(void *log_ctx);

/*
 * Convert a jstring to its utf characters equivalent.
 *
 * @param env JNI environment
 * @param string Java string to convert
 * @param log_ctx context used for logging, can be NULL
 * @return a pointer to an array of unicode characters on success, NULL
 * otherwise
 */
char *js_jni_jstring_to_utf_chars(JNIEnv *env, jstring string, void *log_ctx);

/*
 * Convert utf chars to its jstring equivalent.
 *
 * @param env JNI environment
 * @param utf_chars a pointer to an array of unicode characters
 * @param log_ctx context used for logging, can be NULL
 * @return a Java string object on success, NULL otherwise
 */
jstring js_jni_utf_chars_to_jstring(JNIEnv *env, const char *utf_chars, void *log_ctx);

/*
 * Extract the error summary from a jthrowable in the form of "className: errorMessage"
 *
 * @param env JNI environment
 * @param exception exception to get the summary from
 * @param error address pointing to the error, the value is updated if a
 * summary can be extracted
 * @param log_ctx context used for logging, can be NULL
 * @return 0 on success, < 0 otherwise
 */
int js_jni_exception_get_summary(JNIEnv *env, jthrowable exception, char **error, void *log_ctx);

/*
 * Check if an exception has occurred,log it using av_log and clear it.
 *
 * @param env JNI environment
 * @param log value used to enable logging if an exception has occurred,
 * 0 disables logging, != 0 enables logging
 * @param log_ctx context used for logging, can be NULL
 */
int js_jni_exception_check(JNIEnv *env, int log, void *log_ctx);

/*
 * Jni field type.
 */
enum JSJniFieldType {

    JS_JNI_CLASS,
    JS_JNI_FIELD,
    JS_JNI_STATIC_FIELD,
    JS_JNI_METHOD,
    JS_JNI_STATIC_METHOD

};

/*
 * Jni field describing a class, a field or a method to be retrieved using
 * the js_jni_init_jfields method.
 */
struct JSJniField {

    const char *name;
    const char *method;
    const char *signature;
    enum JSJniFieldType type;
    int offset;
    int mandatory;

};

/*
 * Retrieve class references, field ids and method ids to an arbitrary structure.
 *
 * @param env JNI environment
 * @param jfields a pointer to an arbitrary structure where the different
 * fields are declared and where the FFJNIField mapping table offsets are
 * pointing to
 * @param jfields_mapping null terminated array of FFJNIFields describing
 * the class/field/method to be retrieved
 * @param global make the classes references global. It is the caller
 * responsibility to properly release global references.
 * @param log_ctx context used for logging, can be NULL
 * @return 0 on success, < 0 otherwise
 */
int js_jni_init_jfields(JNIEnv *env, void *jfields, const struct JSJniField *jfields_mapping,
                        int global, void *log_ctx);

/*
 * Delete class references, field ids and method ids of an arbitrary structure.
 *
 * @param env JNI environment
 * @param jfields a pointer to an arbitrary structure where the different
 * fields are declared and where the JSJNIField mapping table offsets are
 * pointing to
 * @param jfields_mapping null terminated array of JSJNIFields describing
 * the class/field/method to be deleted
 * @param global threat the classes references as global and delete them
 * accordingly
 * @param log_ctx context used for logging, can be NULL
 * @return 0 on success, < 0 otherwise
 */
int js_jni_reset_jfields(JNIEnv *env, void *jfields, const struct JSJniField *jfields_mapping,
                         int global, void *log_ctx);

#endif /* JS_JNI_H */
