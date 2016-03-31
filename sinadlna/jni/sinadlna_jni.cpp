#include <assert.h>
#include <jni.h>
#include <android/log.h>
#include <string.h>
#include <sys/types.h>
#include <pthread.h>
#include "sinadlna.h"
#include "sinadlna_jni.h"
#include "sinadlna_notify.h"

#ifndef NELEM
# define NELEM(x) ((int) (sizeof(x) / sizeof((x)[0])))
#endif


struct SinaDLNAClassFields {
    jfieldID    context;
};

static const char* const kClassPathName 	= 	"com/sina/sinavideo/dlna/SinaDLNA";

static SinaDLNAClassFields gSinaDLNAClassFields = {0};
static pthread_mutex_t* gSinaDLNAObjectLock = NULL;
static JavaVM* gJavaVM = NULL;

#if SINA_DLNA_ANDROID
NPT_SET_LOCAL_LOGGER("platinum.android.jni")
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Register native JNI-callable methods.
 *
 * "className" looks like "java/lang/String".
 */
int jniRegisterNativeMethods(JNIEnv* env, const char* className,
    const JNINativeMethod* gMethods, int numMethods)
{
    jclass clazz;
    clazz = env->FindClass( className);
    if (clazz == NULL) {
        NPT_LOG_INFO_1("Native registration unable to find class '%s'\n", className);
        return -1;
    }
    if (env->RegisterNatives(clazz, gMethods, numMethods) < 0) {
        NPT_LOG_INFO_1("RegisterNatives failed for '%s'\n", className);
        return -1;
    }
    return 0;
}


/*
 * Throw an exception with the specified class and an optional message.
 */
int jniThrowException(JNIEnv* env, const char* className, const char* msg)
{
    jclass exceptionClass;

    exceptionClass = env->FindClass( className);
    if (exceptionClass == NULL) {
        NPT_LOG_INFO_1("Unable to find exception class %s\n", className);
        return -1;
    }

    if (env->ThrowNew( exceptionClass, msg) != JNI_OK) {
        NPT_LOG_INFO_2("Failed throwing '%s' '%s'\n", className, msg);
		return -1;
    }
    return 0;
}

#ifdef __cplusplus
}
#endif



/*----------------------------------------------------------------------
|   functions
+---------------------------------------------------------------------*/

static pthread_mutex_t* SinaDLNA_CreateMutex()
{
	pthread_mutex_t* mtx = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
	if(mtx==NULL){
		return NULL;
	}
	int res = pthread_mutex_init(mtx,NULL);
	if(0!=res){
		return NULL;
	}
	
	return mtx;	   
}

static int SinaDLNA_LockMutex(pthread_mutex_t* mtx)
{
	return pthread_mutex_lock(mtx);
}

static int SinaDLNA_UnlockMutex(pthread_mutex_t* mtx)
{
	return pthread_mutex_unlock(mtx);
}

static int SinaDLNA_DestroyMutex(pthread_mutex_t* mtx)
{
	if(mtx){
		pthread_mutex_destroy(mtx);
		free(mtx);
		return 0;
	}
	return -1;
}

static SinaDLNA* getSinaDLNA(JNIEnv* env, jobject thiz)
{
    if(gSinaDLNAObjectLock)
		SinaDLNA_LockMutex(gSinaDLNAObjectLock);
    SinaDLNA* p = (SinaDLNA*)env->GetIntField(thiz, gSinaDLNAClassFields.context);
	if(gSinaDLNAObjectLock)
		SinaDLNA_UnlockMutex(gSinaDLNAObjectLock);
    return p;
}

static SinaDLNA* setSinaDLNA(JNIEnv* env, jobject thiz, SinaDLNA* context)
{
    if(gSinaDLNAObjectLock)
		SinaDLNA_LockMutex(gSinaDLNAObjectLock);
    SinaDLNA* old = (SinaDLNA*)env->GetIntField(thiz, gSinaDLNAClassFields.context);
    env->SetIntField(thiz, gSinaDLNAClassFields.context, (int)context);
	if(gSinaDLNAObjectLock)
		SinaDLNA_UnlockMutex(gSinaDLNAObjectLock);
    return old;
}

static void process_sinadlna_call(JNIEnv *env,int ret)
{
	if ( ret == SinaDLNA::SINA_DLNA_CMD_RESULT_ILLEGALSTATE ) {
		jniThrowException(env, "java/lang/IllegalStateException", NULL);
	} else if ( ret == SinaDLNA::SINA_DLNA_CMD_RESULT_NO_MEMORY ) {
		jniThrowException(env, "java/lang/RuntimeException", NULL);
	} else if ( ret == SinaDLNA::SINA_DLNA_CMD_RESULT_ILLEGALARGUMENT ) {
	    jniThrowException(env, "java/lang/IllegalArgumentException", NULL);
	} 
}
	
static void _init(JNIEnv *env,jobject thiz)
{
	NPT_LOG_INFO_1("%s into",__FUNCTION__);
	
	jclass clazz;

    clazz = env->FindClass(kClassPathName);
    if (clazz == NULL) {
        return;
    }

    gSinaDLNAClassFields.context = env->GetFieldID(clazz, "mNativeContext", "I");
    if (gSinaDLNAClassFields.context == NULL) {
        return;
    }
	
	if(gSinaDLNAObjectLock==NULL){
		gSinaDLNAObjectLock =  SinaDLNA_CreateMutex();
	}
}

static void _setup(JNIEnv *env, jobject thiz, jobject weak_this)
{
    NPT_LOG_INFO_1("%s into",__FUNCTION__);
	
	SinaDLNA* dlna; 
	
	dlna = SinaDLNAHelper::create();
	if(NULL==dlna){
		jniThrowException(env, "java/lang/RuntimeException", "Out of memory");
        return;
	}
	
	NPT_LOG_INFO_1("%s call dlna->setup",__FUNCTION__);
	int ret = dlna->setup(gJavaVM, thiz, weak_this);
	NPT_LOG_INFO_2("%s call dlna->setup, ret=%d",__FUNCTION__,ret);
	
	// Stow our new C++ SinaDLNA in an opaque field in the Java object.
    if(SinaDLNA::SINA_DLNA_CMD_RESULT_SUCCESS==ret){
		setSinaDLNA(env, thiz, dlna);
    }
	
	process_sinadlna_call(env,ret);
	
	NPT_LOG_INFO_1("%s call dlna->begin",__FUNCTION__);
	ret =  dlna->begin();
	NPT_LOG_INFO_2("%s call dlna->begin, ret=%d",__FUNCTION__,ret);
	
	process_sinadlna_call(env,ret);
	
	
}


static void _release(JNIEnv *env, jobject thiz)
{
    NPT_LOG_INFO_1("%s into",__FUNCTION__);
	int ret ;
    SinaDLNA* dlna = setSinaDLNA(env, thiz, 0);
    if (dlna != NULL) {
		NPT_LOG_INFO_1("%s call dlna->end",__FUNCTION__);
		ret = dlna->end();
		NPT_LOG_INFO_2("%s call dlna->end, ret=%d",__FUNCTION__,ret);
		
		NPT_LOG_INFO_1("%s call dlna->release",__FUNCTION__);
		ret = dlna->release();
		NPT_LOG_INFO_2("%s call dlna->release, ret=%d",__FUNCTION__,ret);
		SinaDLNAHelper::destroy(dlna);
    }
}

static void _finalize(JNIEnv * env, jobject thiz)
{
    NPT_LOG_INFO_1("%s into",__FUNCTION__);
	 
	SinaDLNA* dlna = getSinaDLNA(env, thiz);
    if (dlna != NULL) {
        NPT_LOG_INFO_1("%s SinaDLNA finalized without being released", __FUNCTION__);
    }
    _release(env, thiz);
}

static void begin(JNIEnv * env, jobject thiz)
{
    NPT_LOG_INFO_1("%s into",__FUNCTION__); 
	int ret;
    SinaDLNA* dlna = getSinaDLNA(env, thiz);
    if (dlna == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
	
	ret = dlna->begin();

	NPT_LOG_INFO_2("%s result=%d",__FUNCTION__,ret);
	
    process_sinadlna_call(env,ret);
}


static void end(JNIEnv * env, jobject thiz)
{
    NPT_LOG_INFO_1("%s into",__FUNCTION__);
	int ret;
    SinaDLNA* dlna = getSinaDLNA(env, thiz);
    if (dlna == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
	
	ret = dlna->end();

	NPT_LOG_INFO_2("%s result=%d",__FUNCTION__,ret);
	
    process_sinadlna_call(env,ret);
}


static int setMediaRender(JNIEnv * env, jobject thiz, jstring uuid)
{
	NPT_LOG_INFO_1("%s into",__FUNCTION__);
	
	int ret;
    SinaDLNA* dlna = getSinaDLNA(env, thiz);
    if (dlna == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return SinaDLNA::SINA_DLNA_CMD_RESULT_FAIL;
    }
	
	if (uuid == NULL) {
        jniThrowException(env, "java/lang/IllegalArgumentException", NULL);
        return SinaDLNA::SINA_DLNA_CMD_RESULT_FAIL;
    }
	
	const char *tmp = env->GetStringUTFChars(uuid, NULL);
    if (tmp == NULL) {  // Out of memory
		jniThrowException(env, "java/lang/RuntimeException", "Out of memory");
        return SinaDLNA::SINA_DLNA_CMD_RESULT_FAIL;
    }
	
	ret = dlna->setMediaRender(tmp);

	env->ReleaseStringUTFChars(uuid, tmp);
    tmp = NULL;
	
	NPT_LOG_INFO_2("%s result=%d",__FUNCTION__,ret);
	
    process_sinadlna_call(env,ret);
	
	return ret;
}
  
static jstring getMediaRender(JNIEnv * env, jobject thiz)
{
	NPT_LOG_INFO_1("%s into",__FUNCTION__);
	int ret;
    SinaDLNA* dlna = getSinaDLNA(env, thiz);
    if (dlna == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return NULL;
    }
	char uuid[500];

	ret = dlna->getMediaRender(uuid, 500);

	NPT_LOG_INFO_2("%s result=%d",__FUNCTION__,ret);
	
    process_sinadlna_call(env,ret);
	
	if(SinaDLNA::SINA_DLNA_CMD_RESULT_SUCCESS==ret){
		return env->NewStringUTF(uuid);
	}else{
		return NULL;
	}
}

static void open(JNIEnv * env, jobject thiz, jstring url,jstring didl)
{
	NPT_LOG_INFO_1("%s into",__FUNCTION__);
	
	int ret;
    SinaDLNA* dlna = getSinaDLNA(env, thiz);
    if (dlna == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }

	if (url == NULL ) { //didl canbe NULL
        jniThrowException(env, "java/lang/IllegalArgumentException", NULL);
        return;
    }
	
	const char *tmpUrl = env->GetStringUTFChars(url, NULL);
    if (tmpUrl == NULL) {  // Out of memory
		jniThrowException(env, "java/lang/RuntimeException", "url param Out of memory");
        return;
    }
	const char *tmpDidl = NULL;
	if(didl!=NULL){
		NPT_LOG_INFO_1("%s didl not NULL",__FUNCTION__);
		tmpDidl = env->GetStringUTFChars(didl, NULL);
		if (tmpDidl == NULL) {  // Out of memory
			jniThrowException(env, "java/lang/RuntimeException", "didl param Out of memory");
			return;
		}
	}else{
		NPT_LOG_INFO_1("%s didl NULL",__FUNCTION__);
	}
	
	ret = dlna->open(tmpUrl, tmpDidl);

	NPT_LOG_INFO_2("%s result=%d",__FUNCTION__,ret);
	
	env->ReleaseStringUTFChars(url, tmpUrl);
    if(didl!=NULL){
		env->ReleaseStringUTFChars(didl, tmpDidl);
	}
	
    process_sinadlna_call(env,ret);
	
}
  

static void play(JNIEnv * env, jobject thiz)
{
	NPT_LOG_INFO_1("%s into",__FUNCTION__);
	
	int ret;
    SinaDLNA* dlna = getSinaDLNA(env, thiz);
    if (dlna == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
	
	ret = dlna->play();

	NPT_LOG_INFO_2("%s result=%d",__FUNCTION__,ret);
	
    process_sinadlna_call(env,ret);
	
}
  

static void pause(JNIEnv * env, jobject thiz)
{
	NPT_LOG_INFO_1("%s into",__FUNCTION__);
	
	int ret;
    SinaDLNA* dlna = getSinaDLNA(env, thiz);
    if (dlna == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
	
	ret = dlna->pause();

	NPT_LOG_INFO_2("%s result=%d",__FUNCTION__,ret);
	
    process_sinadlna_call(env,ret);
	
}

static void seek(JNIEnv * env, jobject thiz, jint msec)
{
	NPT_LOG_INFO_2("%s into, msec=%d",__FUNCTION__,msec);
	int ret;
    SinaDLNA* dlna = getSinaDLNA(env, thiz);
    if (dlna == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
	
	ret = dlna->seek(msec);

	NPT_LOG_INFO_2("%s result=%d",__FUNCTION__,ret);
	
    process_sinadlna_call(env,ret);
}
  

static void stop(JNIEnv * env, jobject thiz)
{
	NPT_LOG_INFO_1("%s into",__FUNCTION__);
	int ret;
    SinaDLNA* dlna = getSinaDLNA(env, thiz);
    if (dlna == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
	
	ret = dlna->stop();

	NPT_LOG_INFO_2("%s result=%d",__FUNCTION__,ret);
	
    process_sinadlna_call(env,ret);
}



static void setMute(JNIEnv * env, jobject thiz, jboolean mute)
{
	NPT_LOG_INFO_2("%s into, mute=%d",__FUNCTION__,mute);
	int ret;
    SinaDLNA* dlna = getSinaDLNA(env, thiz);
    if (dlna == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
	
	ret = dlna->setMute(mute);

	NPT_LOG_INFO_2("%s result=%d",__FUNCTION__,ret);
	
    process_sinadlna_call(env,ret);
}


static void getMute(JNIEnv * env, jobject thiz)
{
    NPT_LOG_INFO_1("%s into, ",__FUNCTION__);
	int ret;
    SinaDLNA* dlna = getSinaDLNA(env, thiz);
    if (dlna == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
	
	ret = dlna->getMute();

	NPT_LOG_INFO_2("%s result=%d",__FUNCTION__,ret);
	
    process_sinadlna_call(env,ret);
}


static jint getVolumeMin(JNIEnv * env, jobject thiz)
{
	NPT_LOG_INFO_1("%s into, ",__FUNCTION__);
	int ret;
    SinaDLNA* dlna = getSinaDLNA(env, thiz);
    if (dlna == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return -1;
    }
	int volMin = -1, volMax = -1;
	ret = dlna->getVolumeRange(volMin,volMax);

	NPT_LOG_INFO_3("%s result=%d,volMin=%d",__FUNCTION__,ret,volMin);
	
    process_sinadlna_call(env,ret);
	
	return volMin;
}

static jint getVolumeMax(JNIEnv * env, jobject thiz)
{
	NPT_LOG_INFO_1("%s into, ",__FUNCTION__);
	int ret;
    SinaDLNA* dlna = getSinaDLNA(env, thiz);
    if (dlna == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return -1;
    }
	int volMin = -1, volMax = -1;
	ret = dlna->getVolumeRange(volMin,volMax);

	NPT_LOG_INFO_3("%s result=%d,volMax=%d",__FUNCTION__,ret,volMax);
	
    process_sinadlna_call(env,ret);
	
	return volMax;
}


static void setVolume(JNIEnv * env, jobject thiz, jint vol)
{
	NPT_LOG_INFO_2("%s into, vol=%d",__FUNCTION__,vol);
	int ret;
    SinaDLNA* dlna = getSinaDLNA(env, thiz);
    if (dlna == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
	
	ret = dlna->setVolume(vol);

	NPT_LOG_INFO_2("%s result=%d",__FUNCTION__,ret);
	
    process_sinadlna_call(env,ret);
	
}

static void getVolume(JNIEnv * env, jobject thiz)
{
    NPT_LOG_INFO_1("%s into",__FUNCTION__);
	int ret;
    SinaDLNA* dlna = getSinaDLNA(env, thiz);
    if (dlna == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
	
	ret = dlna->getVolume();

	NPT_LOG_INFO_2("%s result=%d",__FUNCTION__,ret);
	
    process_sinadlna_call(env,ret);
}

static void getDuration(JNIEnv * env, jobject thiz)
{
    NPT_LOG_INFO_1("%s into",__FUNCTION__);
	int ret;
    SinaDLNA* dlna = getSinaDLNA(env, thiz);
    if (dlna == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
	
	ret = dlna->getDuration();

	NPT_LOG_INFO_2("%s result=%d",__FUNCTION__,ret);
	
    process_sinadlna_call(env,ret);
}

static void getPosition(JNIEnv * env, jobject thiz)
{
    NPT_LOG_INFO_1("%s into",__FUNCTION__);
	int ret;
    SinaDLNA* dlna = getSinaDLNA(env, thiz);
    if (dlna == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
	
	ret = dlna->getPosition();

	NPT_LOG_INFO_2("%s result=%d",__FUNCTION__,ret);
	
    process_sinadlna_call(env,ret);
}

static JNINativeMethod gMethods[] = {
	{"_init",        		"()V",            								(void *)_init},
	{"_setup",         		"(Ljava/lang/Object;)V",         				(void *)_setup},
	{"_release",        	"()V",            								(void *)_release},
	{"_finalize",     		"()V",                              			(void *)_finalize},
//	{"begin",        		"()V",            								(void *)begin},
//	{"end",        			"()V",            								(void *)end},
    {"setMediaRender", 		"(Ljava/lang/String;)I",						(void *)setMediaRender},
    {"getMediaRender",    	"()Ljava/lang/String;",    						(void *)getMediaRender},
    {"open",    			"(Ljava/lang/String;Ljava/lang/String;)V",    	(void *)open},
    {"play",            	"()V",                              			(void *)play},
	{"pause",            	"()V",                              			(void *)pause},
	{"seek",            	"(I)V",                              			(void *)seek},
	{"stop",            	"()V",                              			(void *)stop},
	{"setMute",            	"(Z)V",                              			(void *)setMute},
	{"getMute",            	"()V",                              			(void *)getMute},
	{"getVolumeMin",        "()I",                              			(void *)getVolumeMin},
	{"getVolumeMax",        "()I",                              			(void *)getVolumeMax},
	{"setVolume",           "(I)V",                              			(void *)setVolume},
	{"getVolume",           "()V",                              			(void *)getVolume},
	{"getDuration",         "()V",                              			(void *)getDuration},
	{"getPosition",         "()V",                              			(void *)getPosition},
};


// This function only registers the native methods
static int register_com_sina_sinavideo_dlna_SinaDLNA(JNIEnv *env)
{
    return jniRegisterNativeMethods(env,kClassPathName, gMethods, NELEM(gMethods));
}


/*----------------------------------------------------------------------
|    JNI_OnLoad
+---------------------------------------------------------------------*/
jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    JNIEnv* env = NULL;
    jint result = -1;

	if(vm==NULL){
		goto bail;
	}
	
	gJavaVM = vm;

    if ( vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        goto bail;
    }
	if(env == NULL){
		goto bail;
	}

	NPT_LogManager::GetDefault().Configure("plist:.level=FINE;.handlers=ConsoleHandler;.ConsoleHandler.outputs=2;.ConsoleHandler.colors=false;.ConsoleHandler.filter=59");
	
    if (register_com_sina_sinavideo_dlna_SinaDLNA(env) < 0) {
        goto bail;
    }
	
    // success -- return valid version number
    result = JNI_VERSION_1_4;

bail:
    return result;
}


void JNI_OnUnload(JavaVM* vm, void* reserved)
{
	
	if(gSinaDLNAObjectLock!=NULL){
		SinaDLNA_DestroyMutex(gSinaDLNAObjectLock);
		gSinaDLNAObjectLock = NULL;
	}
 	
}

