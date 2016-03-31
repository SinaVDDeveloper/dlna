#ifndef _SINAVIDEO_DLNA_H_
#define _SINAVIDEO_DLNA_H_

#include "Platinum.h"
#include "PltMicroMediaController.h"
#include "PltMediaRenderer.h"
#include "PltVersion.h"
#include "sinadlna_notify.h"
#include "sinadlna_listener.h"
#include "sinadlna_config.h"

class SinaDLNA : public SinaDLNAListener{
public:
	//dlna state  machine 状态机
	enum SINA_DLNA_STATE {
		SINA_DLNA_STATE_IDLE  			= 0,   //DLNA 未开始
		SINA_DLNA_STATE_SETUP 			= 1,   //DLNA 初始化
		SINA_DLNA_STATE_START 			= 2,  //DLNA 启动
		SINA_DLNA_STATE_STOP 			= 3,  //DLNA 停止
		SINA_DLNA_STATE_RELEASE 		= 4   //DLNA 释放
	};
	//dlna command result 命令结果
	enum SINA_DLNA_CMD_RESULT {
		SINA_DLNA_CMD_RESULT_ILLEGALARGUMENT = -4,
		SINA_DLNA_CMD_RESULT_NO_MEMORY = -3,
		SINA_DLNA_CMD_RESULT_ILLEGALSTATE = -2,
		SINA_DLNA_CMD_RESULT_FAIL 		= -1,  //命令失败
		SINA_DLNA_CMD_RESULT_SUCCESS  	= 0,  //命令成功，结果马上返回
		SINA_DLNA_CMD_RESULT_WAIT  		= 1   //命令成功，结果需要事件来通知
	};
	SinaDLNA();
	virtual ~SinaDLNA();
#if SINA_DLNA_ANDROID
	int setup(JavaVM* jvm, jobject thiz, jobject weak_thiz);  //dlna stetup DLNA 资源初始化
#endif
#if SINA_DLNA_IOS
	int setup();  //dlna stetup DLNA 资源初始化
#endif
	int release(); //dlna release DLNA 资源释放
	int begin(); //dlna start DLNA 开始工作
	int end();  //dlna stop  DLNA 停止工作
	int setMediaRender(const char* uuid);   //choose a media render 设置当前播放设备
	int getMediaRender(char* uuid, int len); //get current media render
	int open(const char* url, const char* didl);  //open play url
	int play(); //start play
	int stop();  //stop play
	int pause();  //pause play
	int seek(int msec); //seek to msec position
	int setMute(bool yes);  //set mute/unmute voice
	int getMute();  //get current mute state
	int setVolume(int vol);  //set volume
    int getVolume();     //get current volume
	int getVolumeRange(int& volMin, int& volMax);     //get volume range
	int getDuration(); //get media duration, ms
    int getPosition();  //get current position, ms
	
	int OnMREvent(SINA_DLNA_MR_EVENT event,const char* uuid, const char* name);
	int OnMRStateChanged(const char* name, const char* value);
	int OnCommandEvent(SINA_DLNA_CMD_EVENT event, int result, int value, void* data);
private:
	
	SINA_DLNA_STATE state;
	PLT_UPnP *upnp;
	PLT_CtrlPointReference ctrlPoint;
	PLT_MicroMediaController *controller;
	JNISinaDLNAEventNotify* notify;
};

class SinaDLNAHelper
{
public:
	static SinaDLNA* create();
	static void destroy(SinaDLNA* dlna);
};
#endif
