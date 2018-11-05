package com.joyshow.jsplayer.utils;

public class StackTraceInfo {
    /**
     * 打印日志时获取当前的程序文件名、行号、方法名 输出格式为：[FileName | LineNumber | MethodName]
     *
     * @return
     */
    public static String getStackTrace() {
        StackTraceElement traceElement = ((new Exception()).getStackTrace())[1];
        StringBuffer sb = new StringBuffer("[").append(
                traceElement.getFileName()).append(" | ").append(
                traceElement.getLineNumber()).append(" | ").append(
                traceElement.getMethodName()).append("]");
        return sb.toString();
    }

    // 当前文件名
    public static String getFileName() {
        StackTraceElement traceElement = ((new Exception()).getStackTrace())[1];
        return traceElement.getFileName();
    }

    // 当前方法名
    public static String getMethodName() {
        StackTraceElement traceElement = ((new Exception()).getStackTrace())[1];
        return traceElement.getMethodName();
    }

    // 当前行号
    public static int getLineNumber() {
        StackTraceElement traceElement = ((new Exception()).getStackTrace())[1];
        return traceElement.getLineNumber();
    }

}
