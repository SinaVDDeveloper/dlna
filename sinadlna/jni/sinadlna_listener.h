#ifndef _SINAVIDEO_DLNA_LISTENER_H_
#define _SINAVIDEO_DLNA_LISTENER_H_


class SinaDLNAListener{
public:
	//dlna media render event 
	enum SINA_DLNA_MR_EVENT {
		SINA_DLNA_MR_EVENT_ADDED 		= 1,  	//添加一个新的设备
		SINA_DLNA_MR_EVENT_REMOVED 		= 2 	//移除一个已有设备
	};
	//dlna command result event 
	enum SINA_DLNA_CMD_EVENT {
		SINA_DLNA_CMD_EVENT_OPEN 		= 1,  //设置播放URL命令的结果事件
		SINA_DLNA_CMD_EVENT_PLAY 		= 2,  //播放开始命令的结果事件
		SINA_DLNA_CMD_EVENT_PAUSE 		= 3,  //播放暂停命令的结果事件
		SINA_DLNA_CMD_EVENT_STOP 		= 4,  //播放停止命令的结果事件
		SINA_DLNA_CMD_EVENT_SEEK 		= 5,  //播放进度命令的结果事件
		SINA_DLNA_CMD_EVENT_SET_MUTE 	= 6,  //静音设置命令的结果事件
		SINA_DLNA_CMD_EVENT_GET_MUTE 	= 7,  //获得静音设置命令的结果事件
		SINA_DLNA_CMD_EVENT_SET_VOLUME 	= 8,  //音量设置命令的结果事件
		SINA_DLNA_CMD_EVENT_GET_VOLUME 	= 9,  //获得音量设置命令的结果事件
		SINA_DLNA_CMD_EVENT_DURATION 	= 10, //获得视频总时长命令的结果事件
		SINA_DLNA_CMD_EVENT_POSITION 	= 11  //获得当前时间位置命令的结果事件
	};
	virtual ~SinaDLNAListener() {}
    virtual int OnMREvent(SINA_DLNA_MR_EVENT event,const char* uuid, const char* name) = 0;
	virtual int OnMRStateChanged(const char* name, const char* value) = 0;
	virtual int OnCommandEvent(SINA_DLNA_CMD_EVENT, int result, int value, void* data) = 0;
	
};

#endif
