#ifndef _SINAVIDEO_DLNA_EVENT_NOTIFY_H
#define _SINAVIDEO_DLNA_EVENT_NOTIFY_H

#include "sinadlna_config.h"

/************************************************
* DLNA命令事件通知类
*	通知 ANDROID 或 IOS 上层DLNA命令的结果
*************************************************/

#if SINA_DLNA_ANDROID
#include <jni.h>
#endif


//ANDROID平台的事件通知类  
#if SINA_DLNA_ANDROID
class JNISinaDLNAEventNotify
{
public:
    JNISinaDLNAEventNotify(JavaVM* jvm_, jobject thiz, jobject weak_thiz);
    ~JNISinaDLNAEventNotify();
    void postCmdResultEvent(int what, int result, int value);
	void postMediaRenderEvent(int what, const char* uuid, const char* name);
	void postMediaRenderStateChanged(const char* name,const char*  value);
private:
    JNISinaDLNAEventNotify(){}
    jclass      mClass;     // Reference to SinaDLNA class
    jobject     mObject;    // Weak ref to SinaDLNA Java object to call on
    JavaVM* 	jvm;
	jmethodID   post_mr_event;
	jmethodID   post_mr_state;
    jmethodID   post_cmd_event;
};
#endif
  

//IOS平台的事件通知类  
#if SINA_DLNA_IOS
class JNISinaDLNAEventNotify
{
public:
    JNISinaDLNAEventNotify(){}
    ~JNISinaDLNAEventNotify(){}
	void postCmdResultEvent(int what, int result, int value){}
	void postMediaRenderEvent(int what, const char* uuid, const char* name){}

private:
	
};
#endif 


#endif
