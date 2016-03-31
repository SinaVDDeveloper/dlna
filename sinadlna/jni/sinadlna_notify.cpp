#include "sinadlna_notify.h"
#include <cstddef>
#include "NptLogging.h"

#if SINA_DLNA_ANDROID

NPT_SET_LOCAL_LOGGER("platinum.android.jni")

JNISinaDLNAEventNotify::JNISinaDLNAEventNotify(JavaVM* jvm_, jobject thiz, jobject weak_thiz)
{
	bool isAttached = false;  
	JNIEnv* env = NULL;  
	jvm = jvm_;
	if (jvm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {    
		jint res = jvm->AttachCurrentThread(&env, NULL);     
		if ((res < 0) || !env) {      
			return ;    
		}    
		isAttached = true;  
	}

	 
    jclass clazz = env->GetObjectClass(thiz);
    if (clazz == NULL) {
        return;
    }
    mClass = (jclass)env->NewGlobalRef(clazz);

    mObject  = env->NewGlobalRef(weak_thiz);
	
	
	this->post_mr_event = env->GetStaticMethodID(clazz, "postMediaRenderEvent",
                                               "(Ljava/lang/Object;ILjava/lang/String;Ljava/lang/String;)V");
    if (this->post_mr_event == NULL) {
    }
	
	this->post_cmd_event = env->GetStaticMethodID(clazz, "postCmdResultEvent",
                                               "(Ljava/lang/Object;III)V");
    if (this->post_cmd_event == NULL) {
    }
	
	this->post_mr_state = env->GetStaticMethodID(clazz, "postMediaRenderStateChanged",
                                               "(Ljava/lang/Object;Ljava/lang/String;Ljava/lang/String;)V");
    if (this->post_mr_state == NULL) {
    }
	
	
	if (isAttached) {
		if (jvm->DetachCurrentThread() < 0) {
		}
	}
}

JNISinaDLNAEventNotify::~JNISinaDLNAEventNotify()
{
	bool isAttached = false;  
	JNIEnv* env = NULL;  
	if (jvm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {    
		jint res = jvm->AttachCurrentThread(&env, NULL);     
		if ((res < 0) || !env) {      
			return ;    
		}    
		isAttached = true;  
	}
	 
    env->DeleteGlobalRef(mObject);
    env->DeleteGlobalRef(mClass);

	if (isAttached) {
		if (jvm->DetachCurrentThread() < 0) {
		}
	}
}


void JNISinaDLNAEventNotify::postCmdResultEvent(int what, int result, int value)
{
	NPT_LOG_INFO_4("%s into, what=%d,result=%d,value=%d",__FUNCTION__,what, result,value);
	
	if(this->post_cmd_event==NULL){
		return;
	}
    bool isAttached = false;  
	JNIEnv* env = NULL;  
	if (jvm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {    
		jint res = jvm->AttachCurrentThread(&env, NULL);     
		if ((res < 0) || !env) {    
			return ;    
		}    
		isAttached = true;  
	}

	env->CallStaticVoidMethod(mClass, post_cmd_event, mObject, what, result, value);

	if (isAttached) {
		if (jvm->DetachCurrentThread() < 0) {

		}
	}
}


void JNISinaDLNAEventNotify::postMediaRenderEvent(int what, const char* uuid, const char* name)
{
	NPT_LOG_INFO_4("%s into, what=%d,uuid=%s,name=%s",__FUNCTION__,what, uuid,name);
	
	if(this->post_mr_event==NULL){
		return;
	}
	bool isAttached = false;  
	JNIEnv* env = NULL;  
	if (jvm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {    
		jint res = jvm->AttachCurrentThread(&env, NULL);     
		if ((res < 0) || !env) {    
			return ;    
		}    
		isAttached = true;  
	}
	

	env->CallStaticVoidMethod(mClass, post_mr_event, mObject, what, env->NewStringUTF(uuid), env->NewStringUTF(name));

	if (isAttached) {
		if (jvm->DetachCurrentThread() < 0) {
		}
	}
}

void JNISinaDLNAEventNotify::postMediaRenderStateChanged(const char* name,const char*  value)
{
	NPT_LOG_INFO_3("%s into, name=%s,value=%s",__FUNCTION__, name,value);
	
	if(this->post_mr_state==NULL){
		return;
	}
	bool isAttached = false;  
	JNIEnv* env = NULL;  
	if (jvm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {    
		jint res = jvm->AttachCurrentThread(&env, NULL);     
		if ((res < 0) || !env) {    
			return ;    
		}    
		isAttached = true;  
	}
	

	env->CallStaticVoidMethod(mClass, post_mr_state, mObject, env->NewStringUTF(name), env->NewStringUTF(value));

	if (isAttached) {
		if (jvm->DetachCurrentThread() < 0) {
		}
	}
}

#endif