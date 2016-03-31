#include <assert.h>
#include <string.h>
#include <sys/types.h>

#include "sinadlna.h"

#if SINA_DLNA_ANDROID
NPT_SET_LOCAL_LOGGER("platinum.android.jni")
#endif

SinaDLNA* SinaDLNAHelper::create()
{
	SinaDLNA* dlna = new SinaDLNA();
	return dlna;
}

void SinaDLNAHelper::destroy(SinaDLNA* dlna)
{
	if(dlna){
		delete dlna;
	}
}
	
SinaDLNA::SinaDLNA()
{
	this->upnp = NULL;
	this->ctrlPoint = NULL;
	this->controller = NULL;
	this->state = SINA_DLNA_STATE_IDLE;
	this->notify = NULL;
}

SinaDLNA::~SinaDLNA()
{
	release();
}


#if SINA_DLNA_ANDROID
int SinaDLNA::setup(JavaVM* jvm, jobject thiz, jobject weak_thiz)
{
    NPT_LOG_INFO_1("%s into.",__FUNCTION__);
	
	if(this->state==SINA_DLNA_STATE_SETUP){
		NPT_LOG_INFO_1("%s already in setup.",__FUNCTION__);
		return SINA_DLNA_CMD_RESULT_SUCCESS;
	}
	if(this->state!=SINA_DLNA_STATE_IDLE){
		NPT_LOG_INFO_2("%s wrong state. state=%d",__FUNCTION__,this->state);
		return SINA_DLNA_CMD_RESULT_ILLEGALSTATE;
	}
	
	if(NULL==this->upnp){
		this->upnp = new PLT_UPnP();
		if(!this->upnp){
			NPT_LOG_INFO_1("%s no enough memory",__FUNCTION__);
			return SINA_DLNA_CMD_RESULT_NO_MEMORY;
		}
	}
	
	if(this->ctrlPoint.IsNull()){
	  // Create control point
		this->ctrlPoint = new PLT_CtrlPoint();
		if(this->ctrlPoint.IsNull()){
			NPT_LOG_INFO_1("%s no enough memory",__FUNCTION__);
			return SINA_DLNA_CMD_RESULT_NO_MEMORY;
		}
	}
	if(NULL==this->controller){
		// Create controller
		this->controller = new PLT_MicroMediaController(this->ctrlPoint);
		if(!this->controller){
			NPT_LOG_INFO_1("%s no enough memory",__FUNCTION__);
			if(this->upnp){
				delete this->upnp;
				this->upnp = NULL;
			}
			return SINA_DLNA_CMD_RESULT_NO_MEMORY;
		}
		this->controller->setSinaDLNAListener(this);
	}
   
	// add control point to upnp engine and start it
    this->upnp->AddCtrlPoint(this->ctrlPoint);
	
	if(this->notify==NULL){
		this->notify = new JNISinaDLNAEventNotify(jvm, thiz, weak_thiz);
		if(!this->notify){
			NPT_LOG_INFO_1("%s this->notify NULL, no enough memory",__FUNCTION__);
			return SINA_DLNA_CMD_RESULT_NO_MEMORY;
		}
	}
	
	this->state = SINA_DLNA_STATE_SETUP;

	NPT_LOG_INFO_1("%s out.",__FUNCTION__);
	
    return SINA_DLNA_CMD_RESULT_SUCCESS;
}
#endif

#if SINA_DLNA_IOS
int SinaDLNA::setup()
{
    NPT_LOG_INFO_1("%s into.",__FUNCTION__);
	
	if(this->state==SINA_DLNA_STATE_SETUP){
		NPT_LOG_INFO_1("%s already in setup.",__FUNCTION__);
		return SINA_DLNA_CMD_RESULT_SUCCESS;
	}
	if(this->state!=SINA_DLNA_STATE_IDLE){
		NPT_LOG_INFO_2("%s wrong state. state=%d",__FUNCTION__,this->state);
		return SINA_DLNA_CMD_RESULT_ILLEGALSTATE;
	}
	
	if(NULL==this->upnp){
		this->upnp = new PLT_UPnP();
		if(!this->upnp){
			NPT_LOG_INFO_2("%s no enough memory",__FUNCTION__);
			return SINA_DLNA_CMD_RESULT_NO_MEMORY;
		}
	}
	
	if(NULL==this->controller){
		// Create control point
		PLT_CtrlPointReference ctrlPoint(new PLT_CtrlPoint());

		// Create controller
		this->controller = new PLT_MicroMediaController(ctrlPoint);
		if(!this->controller){
			NPT_LOG_INFO_2("%s no enough memory",__FUNCTION__);
			if(this->upnp){
				delete this->upnp;
				this->upnp = NULL;
			}
			return SINA_DLNA_CMD_RESULT_NO_MEMORY;
		}
		this->controller->setSinaDLNAListener(this);
	}
   
	// add control point to upnp engine and start it
    this->upnp->AddCtrlPoint(ctrlPoint);
    
	
	if(this->notify==NULL){
		this->notify = new JNISinaDLNAEventNotify();
	}
	
	this->state = SINA_DLNA_STATE_SETUP;

	NPT_LOG_INFO_1("%s out.",__FUNCTION__);
	
    return SINA_DLNA_CMD_RESULT_SUCCESS;
}
#endif

int SinaDLNA::release()
{
	NPT_LOG_INFO_1("%s into.",__FUNCTION__);
	
	if(this->state==SINA_DLNA_STATE_RELEASE){
		NPT_LOG_INFO_1("%s already in release.",__FUNCTION__);
		return SINA_DLNA_CMD_RESULT_SUCCESS;
	}
	
	end();
	
	NPT_LOG_INFO_1("%s delete controller",__FUNCTION__);
	if(this->controller){
		this->controller->setSinaDLNAListener(NULL);
		delete this->controller;
		this->controller = NULL;
	}
	
	NPT_LOG_INFO_1("%s delete upnp",__FUNCTION__);
	if(this->upnp){
		delete this->upnp;
		this->upnp = NULL;
	}
	
	if(this->notify){
		delete this->notify;
		this->notify = NULL;
	}
	
	this->state = SINA_DLNA_STATE_RELEASE;
	
	NPT_LOG_INFO_1("%s out.",__FUNCTION__);
	
    return SINA_DLNA_CMD_RESULT_SUCCESS;
}


int SinaDLNA::begin()
{
    NPT_LOG_INFO_1("%s into.",__FUNCTION__);
	
	if(this->state==SINA_DLNA_STATE_START){
		NPT_LOG_INFO_1("%s already in start.",__FUNCTION__);
		return SINA_DLNA_CMD_RESULT_SUCCESS;
	}
	if(this->state==SINA_DLNA_STATE_IDLE || this->state==SINA_DLNA_STATE_RELEASE){
		NPT_LOG_INFO_2("%s wrong state. state=%d",__FUNCTION__,this->state);
		return SINA_DLNA_CMD_RESULT_ILLEGALSTATE;
	}
	
    if(this->upnp==NULL){
		NPT_LOG_INFO_1("%s upnp NULL",__FUNCTION__);
		return SINA_DLNA_CMD_RESULT_FAIL;
	}
	
	int ret = this->upnp->Start();
	if(NPT_SUCCESS!=ret){
		NPT_LOG_INFO_1("%s upnp Start fail",__FUNCTION__);
		return SINA_DLNA_CMD_RESULT_FAIL;
	}
	
	this->state = SINA_DLNA_STATE_START;
	
	// tell control point to perform extra broadcast discover  just one time //every 6 secs
    // in case our device doesn't support multicast
	this->ctrlPoint->Discover(NPT_HttpUrl("255.255.255.255", 1900, "*"), "upnp:rootdevice", 1, NPT_TimeInterval(0.) /*6000*/);
    this->ctrlPoint->Discover(NPT_HttpUrl("239.255.255.250", 1900, "*"), "upnp:rootdevice", 1, NPT_TimeInterval(0.) /*6000*/);
	
	NPT_LOG_INFO_1("%s out.",__FUNCTION__);
    
	return SINA_DLNA_CMD_RESULT_SUCCESS;
}


int  SinaDLNA::end()
{
    NPT_LOG_INFO_1("%s into.",__FUNCTION__);
	
	if(this->state==SINA_DLNA_STATE_STOP){
		NPT_LOG_INFO_1("%s already in stop.",__FUNCTION__);
		return SINA_DLNA_CMD_RESULT_SUCCESS;
	}
	if(this->state==SINA_DLNA_STATE_IDLE || this->state==SINA_DLNA_STATE_RELEASE){
		NPT_LOG_INFO_2("%s wrong state. state=%d",__FUNCTION__,this->state);
		return SINA_DLNA_CMD_RESULT_ILLEGALSTATE;
	}
	
    if(this->upnp==NULL){
		NPT_LOG_INFO_1("%s upnp NULL",__FUNCTION__);
		return SINA_DLNA_CMD_RESULT_FAIL;
	}
	
	int ret = this->upnp->Stop();
	if(NPT_SUCCESS!=ret){
		NPT_LOG_INFO_1("%s upnp Stop fail",__FUNCTION__);
		return SINA_DLNA_CMD_RESULT_FAIL;
	}
	
	this->state = SINA_DLNA_STATE_STOP;
	
    NPT_LOG_INFO_2("%s out. ret=%d",__FUNCTION__,ret);
    
	return SINA_DLNA_CMD_RESULT_SUCCESS;
}



int SinaDLNA::setMediaRender(const char* uuid)
{
	NPT_LOG_INFO_1("%s into.",__FUNCTION__);
	if(this->state!=SINA_DLNA_STATE_START){
		NPT_LOG_INFO_2("%s wrong state. state=%d",__FUNCTION__,this->state);
		return SINA_DLNA_CMD_RESULT_ILLEGALSTATE;
	}
	
	if(this->controller==NULL){
		NPT_LOG_INFO_1("%s controller NULL.",__FUNCTION__);
		return SINA_DLNA_CMD_RESULT_FAIL;
	}
	
    this->controller->setmr(uuid);
	
	NPT_LOG_INFO_2("%s out. uuid=%s",__FUNCTION__,uuid);
    
	return SINA_DLNA_CMD_RESULT_SUCCESS;
}

int SinaDLNA::getMediaRender(char* uuid, int len)
{
	NPT_LOG_INFO_1("%s into.",__FUNCTION__);
	if(state!=SINA_DLNA_STATE_START){
		NPT_LOG_INFO_2("%s wrong state. state=%d",__FUNCTION__,this->state);
		return SINA_DLNA_CMD_RESULT_ILLEGALSTATE;
	}
	
	if(this->controller==NULL){
		NPT_LOG_INFO_1("%s controller NULL.",__FUNCTION__);
		return SINA_DLNA_CMD_RESULT_FAIL;
	}
	
    this->controller->getmr(uuid, len);
	
	NPT_LOG_INFO_2("%s out. uuid=%s",__FUNCTION__,uuid);
    
	return SINA_DLNA_CMD_RESULT_SUCCESS;
}
  

  
int SinaDLNA::open(const char* url, const char* didl)
{
	NPT_LOG_INFO_1("%s into.",__FUNCTION__);
	
	if(this->state!=SINA_DLNA_STATE_START){
		NPT_LOG_INFO_2("%s wrong state. state=%d",__FUNCTION__,this->state);
		return SINA_DLNA_CMD_RESULT_ILLEGALSTATE;
	}
	
    if(this->controller==NULL){
		NPT_LOG_INFO_1("%s controller NULL.",__FUNCTION__);
		return SINA_DLNA_CMD_RESULT_FAIL;
	}
	
    this->controller->open(url, didl);
		
	NPT_LOG_INFO_1("%s out.",__FUNCTION__);
	
	return SINA_DLNA_CMD_RESULT_WAIT;
}
  

int SinaDLNA::play()
{
	NPT_LOG_INFO_1("%s into.",__FUNCTION__);
	
	if(this->state!=SINA_DLNA_STATE_START){
		NPT_LOG_INFO_2("%s wrong state. state=%d",__FUNCTION__,this->state);
		return SINA_DLNA_CMD_RESULT_ILLEGALSTATE;
	}
	
    if(this->controller==NULL){
		NPT_LOG_INFO_1("%s controller NULL.",__FUNCTION__);
		return SINA_DLNA_CMD_RESULT_FAIL;
	}
	
    this->controller->play();
		
	NPT_LOG_INFO_1("%s out.",__FUNCTION__);
	
	return SINA_DLNA_CMD_RESULT_WAIT;
}
 

int SinaDLNA::stop()
{
	NPT_LOG_INFO_1("%s into.",__FUNCTION__);
	
	if(this->state!=SINA_DLNA_STATE_START){
		NPT_LOG_INFO_2("%s wrong state. state=%d",__FUNCTION__,this->state);
		return SINA_DLNA_CMD_RESULT_ILLEGALSTATE;
	}
	
    if(this->controller==NULL){
		NPT_LOG_INFO_1("%s controller NULL.",__FUNCTION__);
		return SINA_DLNA_CMD_RESULT_FAIL;
	}
	
    this->controller->stop();
		
	NPT_LOG_INFO_1("%s out.",__FUNCTION__);
	
	return SINA_DLNA_CMD_RESULT_WAIT;
} 


int SinaDLNA::seek(int msec)
{
	NPT_LOG_INFO_1("%s into.",__FUNCTION__);
	
	if(this->state!=SINA_DLNA_STATE_START){
		NPT_LOG_INFO_2("%s wrong state. state=%d",__FUNCTION__,this->state);
		return SINA_DLNA_CMD_RESULT_ILLEGALSTATE;
	}
	
    if(this->controller==NULL){
		NPT_LOG_INFO_1("%s controller NULL.",__FUNCTION__);
		return SINA_DLNA_CMD_RESULT_FAIL;
	}
	
	int hour=0, min=0, sec=0;
	min = (msec/1000)/60;
	if(min>=60){
		hour = min/60;
		min = min%60;
	}else{
		hour = 0;
	}
	sec = (msec/1000)%60;
	
    char strpos[50];
	memset(strpos,0,sizeof(strpos));
	sprintf(strpos,"%02d:%02d:%02d",hour,min,sec);
	
	NPT_LOG_INFO_2("%s seek to %s",__FUNCTION__,strpos);
	
    this->controller->seek(strpos);
		
	NPT_LOG_INFO_1("%s out.",__FUNCTION__);
	
	return SINA_DLNA_CMD_RESULT_WAIT;
}
  

int SinaDLNA::pause()
{
	NPT_LOG_INFO_1("%s into",__FUNCTION__);
	
	if(this->state!=SINA_DLNA_STATE_START){
		NPT_LOG_INFO_2("%s wrong state. state=%d",__FUNCTION__,this->state);
		return SINA_DLNA_CMD_RESULT_ILLEGALSTATE;
	}
	
    if(this->controller==NULL){
		NPT_LOG_INFO_1("%s controller NULL.",__FUNCTION__);
		return SINA_DLNA_CMD_RESULT_FAIL;
	}

    this->controller->pause();
	
	return SINA_DLNA_CMD_RESULT_WAIT;
}


int SinaDLNA::setMute(bool yes)
{
	NPT_LOG_INFO_2("%s into, yes=%d",__FUNCTION__,yes);
	
	if(this->state!=SINA_DLNA_STATE_START){
		NPT_LOG_INFO_2("%s wrong state. state=%d",__FUNCTION__,this->state);
		return SINA_DLNA_CMD_RESULT_ILLEGALSTATE;
	}
	
    if(this->controller==NULL){
		NPT_LOG_INFO_1("%s controller NULL.",__FUNCTION__);
		return SINA_DLNA_CMD_RESULT_FAIL;
	}
	
    this->controller->setMute(yes);
	
	return  SINA_DLNA_CMD_RESULT_WAIT;
}


int SinaDLNA::getMute()
{
    NPT_LOG_INFO_1("%s into",__FUNCTION__);
	
	if(this->state!=SINA_DLNA_STATE_START){
		NPT_LOG_INFO_2("%s wrong state. state=%d",__FUNCTION__,this->state);
		return SINA_DLNA_CMD_RESULT_ILLEGALSTATE;
	}
	
    if(this->controller==NULL){
		NPT_LOG_INFO_1("%s controller NULL.",__FUNCTION__);
		return SINA_DLNA_CMD_RESULT_FAIL;
	}
	
    this->controller->getMute();
	
	return  SINA_DLNA_CMD_RESULT_WAIT;
}


int SinaDLNA::setVolume(int vol)
{
	NPT_LOG_INFO_2("%s into,vol=%d",__FUNCTION__,vol);
	
	if(this->state!=SINA_DLNA_STATE_START){
		NPT_LOG_INFO_2("%s wrong state. state=%d",__FUNCTION__,this->state);
		return SINA_DLNA_CMD_RESULT_ILLEGALSTATE;
	}
	
    if(this->controller==NULL){
		NPT_LOG_INFO_1("%s controller NULL.",__FUNCTION__);
		return SINA_DLNA_CMD_RESULT_FAIL;
	}
	
    this->controller->setVolume(vol);
	
	return  SINA_DLNA_CMD_RESULT_WAIT;
}

int SinaDLNA::getVolume()
{
	NPT_LOG_INFO_1("%s into",__FUNCTION__);
	
	if(this->state!=SINA_DLNA_STATE_START){
		NPT_LOG_INFO_2("%s wrong state. state=%d",__FUNCTION__,this->state);
		return SINA_DLNA_CMD_RESULT_ILLEGALSTATE;
	}
	
    if(this->controller==NULL){
		NPT_LOG_INFO_1("%s controller NULL.",__FUNCTION__);
		return SINA_DLNA_CMD_RESULT_FAIL;
	}
	
    this->controller->getVolume();
	
	return  SINA_DLNA_CMD_RESULT_WAIT;
}

int SinaDLNA::getVolumeRange(int& volMin, int& volMax)
{
	NPT_LOG_INFO_1("%s into",__FUNCTION__);
	
	if(this->state!=SINA_DLNA_STATE_START){
		NPT_LOG_INFO_2("%s wrong state. state=%d",__FUNCTION__,this->state);
		return SINA_DLNA_CMD_RESULT_ILLEGALSTATE;
	}
	
    if(this->controller==NULL){
		NPT_LOG_INFO_1("%s controller NULL.",__FUNCTION__);
		return SINA_DLNA_CMD_RESULT_FAIL;
	}
	
    this->controller->getVolumeRange(volMin,  volMax);
	
	return  SINA_DLNA_CMD_RESULT_SUCCESS;
}

int SinaDLNA::getDuration()
{
	NPT_LOG_INFO_1("%s into",__FUNCTION__);
	
	if(this->state!=SINA_DLNA_STATE_START){
		NPT_LOG_INFO_2("%s wrong state. state=%d",__FUNCTION__,this->state);
		return SINA_DLNA_CMD_RESULT_ILLEGALSTATE;
	}
	
    if(this->controller==NULL){
		NPT_LOG_INFO_1("%s controller NULL.",__FUNCTION__);
		return SINA_DLNA_CMD_RESULT_FAIL;
	}
	
    this->controller->getMediaInfo();
	
	return  SINA_DLNA_CMD_RESULT_WAIT;
}

int SinaDLNA::getPosition()
{
	NPT_LOG_INFO_1("%s into",__FUNCTION__);
	
	if(this->state!=SINA_DLNA_STATE_START){
		NPT_LOG_INFO_2("%s wrong state. state=%d",__FUNCTION__,this->state);
		return SINA_DLNA_CMD_RESULT_ILLEGALSTATE;
	}
	
    if(this->controller==NULL){
		NPT_LOG_INFO_1("%s controller NULL.",__FUNCTION__);
		return SINA_DLNA_CMD_RESULT_FAIL;
	}
	
    this->controller->getPositionInfo();
	
	return  SINA_DLNA_CMD_RESULT_WAIT;
}
	

int SinaDLNA::OnMREvent(SINA_DLNA_MR_EVENT event,const char* uuid, const char* name)
{
	if(this->notify){
		this->notify->postMediaRenderEvent(event,  uuid, name);
	}
}

int SinaDLNA::OnMRStateChanged(const char* name, const char* value)
{
	if(this->notify){
		this->notify->postMediaRenderStateChanged(name, value);
	}
}

int SinaDLNA::OnCommandEvent(SINA_DLNA_CMD_EVENT event, int result, int value, void* data)
{	
	NPT_LOG_INFO_4("%s into, event=%d,result=%d,value=%d",__FUNCTION__,event, result,value);
	if(this->notify){
		this->notify->postCmdResultEvent(event, result, value);
	}
}