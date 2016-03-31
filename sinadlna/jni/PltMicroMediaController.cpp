/*****************************************************************
|
|   Platinum - Miccro Media Controller
|
| Copyright (c) 2004-2010, Plutinosoft, LLC.
| All rights reserved.
| http://www.plutinosoft.com
|
| This program is free software; you can redistribute it and/or
| modify it under the terms of the GNU General Public License
| as published by the Free Software Foundation; either version 2
| of the License, or (at your option) any later version.
|
| OEMs, ISVs, VARs and other distributors that combine and 
| distribute commercially licensed software with Platinum software
| and do not wish to distribute the source code for the commercially
| licensed software under version 2, or (at your option) any later
| version, of the GNU General Public License (the "GPL") must enter
| into a commercial license agreement with Plutinosoft, LLC.
| licensing@plutinosoft.com
| 
| This program is distributed in the hope that it will be useful,
| but WITHOUT ANY WARRANTY; without even the implied warranty of
| MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
| GNU General Public License for more details.
|
| You should have received a copy of the GNU General Public License
| along with this program; see the file LICENSE.txt. If not, write to
| the Free Software Foundation, Inc., 
| 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
| http://www.gnu.org/licenses/gpl-2.0.html
|
****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "PltMicroMediaController.h"
#include "PltLeaks.h"
#include "PltDownloader.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#if SINA_DLNA_ANDROID
NPT_SET_LOCAL_LOGGER("platinum.android.jni")
#endif

/*----------------------------------------------------------------------
|   PLT_MicroMediaController::PLT_MicroMediaController
+---------------------------------------------------------------------*/
PLT_MicroMediaController::PLT_MicroMediaController(PLT_CtrlPointReference& ctrlPoint) :
    //PLT_SyncMediaBrowser(ctrlPoint),
	m_dlna_listener(NULL),
    PLT_MediaController(ctrlPoint),
	m_TrackDuration(-1),
	m_MediaDuration(-1)
{
    // create the stack that will be the directory where the
    // user is currently browsing. 
    // push the root directory onto the directory stack.
    //m_CurBrowseDirectoryStack.Push("0");

    PLT_MediaController::SetDelegate(this);
}

/*----------------------------------------------------------------------
|   PLT_MicroMediaController::PLT_MicroMediaController
+---------------------------------------------------------------------*/
PLT_MicroMediaController::~PLT_MicroMediaController()
{
}


void 	
PLT_MicroMediaController::setSinaDLNAListener(SinaDLNAListener* listener)
{
	NPT_AutoLock lock(m_dlna_listener_lock);
	m_dlna_listener = listener;
}


/*----------------------------------------------------------------------
|   PLT_MicroMediaController::OnMRAdded
+---------------------------------------------------------------------*/
bool
PLT_MicroMediaController::OnMRAdded(PLT_DeviceDataReference& device)
{
	
    NPT_String uuid = device->GetUUID();
	
	NPT_LOG_INFO_2("%s into. uuid=%s", __FUNCTION__, (const char*)uuid);

    // test if it's a media renderer
    PLT_Service* service;
    if (NPT_SUCCEEDED(device->FindServiceByType("urn:schemas-upnp-org:service:AVTransport:*", service))) {
        NPT_AutoLock lock(m_MediaRenderers);
        m_MediaRenderers.Put(uuid, device);
		
		NPT_AutoLock lock2(m_dlna_listener_lock);
		if(m_dlna_listener){
			m_dlna_listener->OnMREvent(SinaDLNAListener::SINA_DLNA_MR_EVENT_ADDED, (const char*)uuid, (const char*)device->GetFriendlyName() );
		}
    }
    
    return true;
}

/*----------------------------------------------------------------------
|   PLT_MicroMediaController::OnMRRemoved
+---------------------------------------------------------------------*/
void
PLT_MicroMediaController::OnMRRemoved(PLT_DeviceDataReference& device)
{
    NPT_String uuid = device->GetUUID();

	NPT_LOG_INFO_2("%s into. uuid=%s", __FUNCTION__, (const char*)uuid);
	
    {
        NPT_AutoLock lock(m_MediaRenderers);
        m_MediaRenderers.Erase(uuid);
		
		NPT_AutoLock lock2(m_dlna_listener_lock);
		if(m_dlna_listener){
			m_dlna_listener->OnMREvent(SinaDLNAListener::SINA_DLNA_MR_EVENT_REMOVED, (const char*)uuid, (const char*)device->GetFriendlyName());
		}
    }

    {
        NPT_AutoLock lock(m_CurMediaRendererLock);

        // if it's the currently selected one, we have to get rid of it
        if (!m_CurMediaRenderer.IsNull() && m_CurMediaRenderer == device) {
            m_CurMediaRenderer = NULL;
        }
    }
}

/*----------------------------------------------------------------------
|   PLT_MicroMediaController::OnMRStateVariablesChanged
+---------------------------------------------------------------------*/
void
PLT_MicroMediaController::OnMRStateVariablesChanged(PLT_Service*                  service,
                                                    NPT_List<PLT_StateVariable*>* vars)
{
	//应该通知设备的状态变化操作
	int notify = 0;
	PLT_DeviceDataReference cur_device;
    GetCurMediaRenderer(cur_device);
	if ( !cur_device.IsNull() && cur_device->GetUUID()==service->GetDevice()->GetUUID() ) {
		notify = 1;
    }
	
    NPT_String uuid = service->GetDevice()->GetUUID();
    NPT_List<PLT_StateVariable*>::Iterator var = vars->GetFirstItem();
    while (var) {
        NPT_LOG_INFO_5("%s Received state var \"%s:%s:%s\" changes: \"%s\"\n",
				__FUNCTION__,
               (const char*)uuid,
               (const char*)service->GetServiceID(),
               (const char*)(*var)->GetName(),
               (const char*)(*var)->GetValue());
		
		if(notify && m_dlna_listener){
			m_dlna_listener->OnMRStateChanged( (const char*)(*var)->GetName(), (const char*)(*var)->GetValue() );
		}
        ++var;
    }
}


// AVTransport
void 
PLT_MicroMediaController::OnGetMediaInfoResult(
        NPT_Result               res,
        PLT_DeviceDataReference& device,
        PLT_MediaInfo*           info ,
        void*                    userdata)
{
	NPT_LOG_INFO_1("%s into.", __FUNCTION__);
	
	PLT_DeviceDataReference cur_device;
    GetCurMediaRenderer(cur_device);
    if (cur_device.IsNull()) {
		NPT_LOG_INFO_1("%s current device null", __FUNCTION__);
        return;
    }
	if( cur_device->GetUUID() != device->GetUUID() ){
		NPT_LOG_INFO_1("%s device not equal current device", __FUNCTION__);
		return;
	}
	
	NPT_AutoLock lock(m_dlna_listener_lock);
	
	if(m_dlna_listener){
		if(info){
			m_MediaDuration = info->media_duration.ToSeconds()*1000;
			if(m_MediaDuration==0){
				NPT_LOG_INFO_1("%s MediaInfo return bad MediaDuration.", __FUNCTION__);
			}
			if(m_MediaDuration==0 && m_TrackDuration>0){
				m_MediaDuration = m_TrackDuration;
				NPT_LOG_INFO_1("%s use PositionInfo TrackDuration instead of MediaDuration.", __FUNCTION__);
				m_dlna_listener->OnCommandEvent(SinaDLNAListener::SINA_DLNA_CMD_EVENT_DURATION, res, m_MediaDuration, NULL) ;
			}else{
				m_dlna_listener->OnCommandEvent(SinaDLNAListener::SINA_DLNA_CMD_EVENT_DURATION, res, info->media_duration.ToSeconds()*1000, NULL) ;
			}
		}else{
			NPT_LOG_INFO_1("%s info NULL, bad result!", __FUNCTION__);
			m_dlna_listener->OnCommandEvent(SinaDLNAListener::SINA_DLNA_CMD_EVENT_DURATION, res, 0 , NULL) ;
		}
	}
}

void 
PLT_MicroMediaController::OnGetPositionInfoResult(
        NPT_Result               res ,
        PLT_DeviceDataReference& device ,
        PLT_PositionInfo*        info ,
        void*                    userdata )
{
	NPT_LOG_INFO_1("%s into.", __FUNCTION__);
	
	PLT_DeviceDataReference cur_device;
    GetCurMediaRenderer(cur_device);
    if (cur_device.IsNull()) {
		NPT_LOG_INFO_1("%s current device null", __FUNCTION__);
        return;
    }
	if( cur_device->GetUUID() != device->GetUUID() ){
		NPT_LOG_INFO_1("%s device not equal current device", __FUNCTION__);
		return;
	}
	
	NPT_AutoLock lock(m_dlna_listener_lock);
	if(m_dlna_listener){
		if(info){
			if(m_MediaDuration==0){
				NPT_LOG_INFO_1("%s MediaInfo return bad MediaDuration.", __FUNCTION__);
				m_TrackDuration = info->track_duration.ToSeconds()*1000;
				if(m_TrackDuration>0){
					m_MediaDuration = m_TrackDuration;
					NPT_LOG_INFO_1("%s use PositionInfo TrackDuration instead of MediaDuration.", __FUNCTION__);
					m_dlna_listener->OnCommandEvent(SinaDLNAListener::SINA_DLNA_CMD_EVENT_DURATION, res, m_MediaDuration, NULL) ;
				}
			}
			m_dlna_listener->OnCommandEvent(SinaDLNAListener::SINA_DLNA_CMD_EVENT_POSITION, res, info->rel_time.ToSeconds()*1000, NULL) ;
		}else{
			NPT_LOG_INFO_1("%s info NULL, bad result!", __FUNCTION__);
			m_dlna_listener->OnCommandEvent(SinaDLNAListener::SINA_DLNA_CMD_EVENT_DURATION, res, 0 , NULL) ;
		}
	}
}
		
void 
PLT_MicroMediaController::OnSetAVTransportURIResult(
        NPT_Result               res ,
        PLT_DeviceDataReference& device ,
        void*                    userdata)
{
	NPT_LOG_INFO_1("%s into.", __FUNCTION__);
	
	PLT_DeviceDataReference cur_device;
    GetCurMediaRenderer(cur_device);
    if (cur_device.IsNull()) {
		NPT_LOG_INFO_1("%s current device null", __FUNCTION__);
        return;
    }
	if( cur_device->GetUUID() != device->GetUUID() ){
		NPT_LOG_INFO_1("%s device not equal current device", __FUNCTION__);
		return;
	}
	
	NPT_AutoLock lock(m_dlna_listener_lock);
	if(m_dlna_listener){
		m_dlna_listener->OnCommandEvent(SinaDLNAListener::SINA_DLNA_CMD_EVENT_OPEN, res, 0, NULL) ;
	}
}

		
void 
PLT_MicroMediaController::OnPlayResult(
        NPT_Result               res ,
        PLT_DeviceDataReference& device ,
        void*                    userdata)
{
	NPT_LOG_INFO_1("%s into.", __FUNCTION__);
	
	PLT_DeviceDataReference cur_device;
    GetCurMediaRenderer(cur_device);
    if (cur_device.IsNull()) {
		NPT_LOG_INFO_1("%s current device null", __FUNCTION__);
        return;
    }
	if( cur_device->GetUUID() != device->GetUUID() ){
		NPT_LOG_INFO_1("%s device not equal current device", __FUNCTION__);
		return;
	}
	
	NPT_AutoLock lock(m_dlna_listener_lock);
	if(m_dlna_listener){
		m_dlna_listener->OnCommandEvent(SinaDLNAListener::SINA_DLNA_CMD_EVENT_PLAY, res, 0, NULL) ;
	}
}

void 
PLT_MicroMediaController::OnPauseResult(
        NPT_Result               res ,
        PLT_DeviceDataReference& device,
        void*                     userdata )
{
	NPT_LOG_INFO_1("%s into.", __FUNCTION__);
	
	PLT_DeviceDataReference cur_device;
    GetCurMediaRenderer(cur_device);
    if (cur_device.IsNull()) {
		NPT_LOG_INFO_1("%s current device null", __FUNCTION__);
        return;
    }
	if( cur_device->GetUUID() != device->GetUUID() ){
		NPT_LOG_INFO_1("%s device not equal current device", __FUNCTION__);
		return;
	}
	
	NPT_AutoLock lock(m_dlna_listener_lock);
	if(m_dlna_listener){
		m_dlna_listener->OnCommandEvent(SinaDLNAListener::SINA_DLNA_CMD_EVENT_PAUSE, res, 0, NULL) ;
	}
}


void 
PLT_MicroMediaController::OnSeekResult(
        NPT_Result               res ,
        PLT_DeviceDataReference& device,
        void*                    userdata)
{
	NPT_LOG_INFO_1("%s into.", __FUNCTION__);
	
	PLT_DeviceDataReference cur_device;
    GetCurMediaRenderer(cur_device);
    if (cur_device.IsNull()) {
		NPT_LOG_INFO_1("%s current device null", __FUNCTION__);
        return;
    }
	if( cur_device->GetUUID() != device->GetUUID() ){
		NPT_LOG_INFO_1("%s device not equal current device", __FUNCTION__);
		return;
	}
	
	NPT_AutoLock lock(m_dlna_listener_lock);
	if(m_dlna_listener){
		m_dlna_listener->OnCommandEvent(SinaDLNAListener::SINA_DLNA_CMD_EVENT_SEEK, res, 0, NULL) ;
	}	
}


void 
PLT_MicroMediaController::OnStopResult(
        NPT_Result               res ,
        PLT_DeviceDataReference& device ,
        void*                    userdata)
{
	NPT_LOG_INFO_1("%s into.", __FUNCTION__);
	
	PLT_DeviceDataReference cur_device;
    GetCurMediaRenderer(cur_device);
    if (cur_device.IsNull()) {
		NPT_LOG_INFO_1("%s current device null", __FUNCTION__);
        return;
    }
	if( cur_device->GetUUID() != device->GetUUID() ){
		NPT_LOG_INFO_1("%s device not equal current device", __FUNCTION__);
		return;
	}
	
	NPT_AutoLock lock(m_dlna_listener_lock);
	if(m_dlna_listener){
		m_dlna_listener->OnCommandEvent(SinaDLNAListener::SINA_DLNA_CMD_EVENT_STOP, res, 0, NULL) ;
	}	
}
	
// RenderingControl
void 
PLT_MicroMediaController::OnSetMuteResult(
        NPT_Result               res,
        PLT_DeviceDataReference& device,
        void*                    userdata)
{
	NPT_LOG_INFO_1("%s into.", __FUNCTION__);
	
	PLT_DeviceDataReference cur_device;
    GetCurMediaRenderer(cur_device);
    if (cur_device.IsNull()) {
		NPT_LOG_INFO_1("%s current device null", __FUNCTION__);
        return;
    }
	if( cur_device->GetUUID() != device->GetUUID() ){
		NPT_LOG_INFO_1("%s device not equal current device", __FUNCTION__);
		return;
	}
	
	NPT_AutoLock lock(m_dlna_listener_lock);
	if(m_dlna_listener){
		m_dlna_listener->OnCommandEvent(SinaDLNAListener::SINA_DLNA_CMD_EVENT_SET_MUTE, res, 0, NULL) ;
	}
}

void 
PLT_MicroMediaController::OnGetMuteResult(
        NPT_Result               res ,
        PLT_DeviceDataReference& device ,
        const char*              channel,
        bool                     mute,
        void*                    userdata) 
{
	NPT_LOG_INFO_1("%s into.", __FUNCTION__);
	
	PLT_DeviceDataReference cur_device;
    GetCurMediaRenderer(cur_device);
    if (cur_device.IsNull()) {
		NPT_LOG_INFO_1("%s current device null", __FUNCTION__);
        return;
    }
	if( cur_device->GetUUID() != device->GetUUID() ){
		NPT_LOG_INFO_1("%s device not equal current device", __FUNCTION__);
		return;
	}
	
	NPT_AutoLock lock(m_dlna_listener_lock);
	if(m_dlna_listener){
		m_dlna_listener->OnCommandEvent(SinaDLNAListener::SINA_DLNA_CMD_EVENT_GET_MUTE, res, mute, NULL) ;
	}
}

void 
PLT_MicroMediaController::OnSetVolumeResult(
        NPT_Result               res ,
        PLT_DeviceDataReference& device,
        void*                    userdata)
{
	NPT_LOG_INFO_1("%s into.", __FUNCTION__);
	
	PLT_DeviceDataReference cur_device;
    GetCurMediaRenderer(cur_device);
    if (cur_device.IsNull()) {
		NPT_LOG_INFO_1("%s current device null", __FUNCTION__);
        return;
    }
	if( cur_device->GetUUID() != device->GetUUID() ){
		NPT_LOG_INFO_1("%s device not equal current device", __FUNCTION__);
		return;
	}
	
	NPT_AutoLock lock(m_dlna_listener_lock);
	if(m_dlna_listener){
		m_dlna_listener->OnCommandEvent(SinaDLNAListener::SINA_DLNA_CMD_EVENT_SET_VOLUME, res, 0, NULL) ;
	}
}

void PLT_MicroMediaController::OnGetVolumeResult(
        NPT_Result               res,
        PLT_DeviceDataReference& device,
		const char*              channel,
    	NPT_UInt32				 volume ,
	    void*                    userdata)
{
	NPT_LOG_INFO_1("%s into.", __FUNCTION__);
	
	PLT_DeviceDataReference cur_device;
    GetCurMediaRenderer(cur_device);
    if (cur_device.IsNull()) {
		NPT_LOG_INFO_1("%s current device null", __FUNCTION__);
        return;
    }
	if( cur_device->GetUUID() != device->GetUUID() ){
		NPT_LOG_INFO_1("%s device not equal current device", __FUNCTION__);
		return;
	}
	
	NPT_AutoLock lock(m_dlna_listener_lock);
	if(m_dlna_listener){
		m_dlna_listener->OnCommandEvent(SinaDLNAListener::SINA_DLNA_CMD_EVENT_GET_VOLUME, res, volume, NULL) ;
	}
}
		
/*----------------------------------------------------------------------
|   PLT_MicroMediaController::GetCurMediaRenderer
+---------------------------------------------------------------------*/
void
PLT_MicroMediaController::GetCurMediaRenderer(PLT_DeviceDataReference& renderer)
{
    NPT_AutoLock lock(m_CurMediaRendererLock);

    if (m_CurMediaRenderer.IsNull()) {
        NPT_LOG_INFO("No renderer selected, select one with setmr\n");
    } else {
        renderer = m_CurMediaRenderer;
    }
}


/*----------------------------------------------------------------------
|   PLT_MicroMediaController::HandleCmd_setmr
+---------------------------------------------------------------------*/
void 
PLT_MicroMediaController::setmr(const char *uuid)
{
	NPT_LOG_INFO_2("setmr uuid=[%s]",__FUNCTION__,uuid);
	
	PLT_DeviceDataReference* result = NULL;
	{
		NPT_AutoLock lock(m_MediaRenderers);
		m_MediaRenderers.Get(uuid, result);
	}
	if(result==NULL){
		NPT_LOG_INFO_2("%s cannot find mr, uuid=[%s]",__FUNCTION__,uuid);
	}else{
		NPT_LOG_INFO_3("%s find mr, uuid=[%s] name=[%s]",__FUNCTION__,uuid,(const char*)(*result)->GetFriendlyName());
	}
	
    NPT_AutoLock lock(m_CurMediaRendererLock);
	m_CurMediaRenderer = result?*result:PLT_DeviceDataReference(); // return empty reference if not device was selected
}

/*----------------------------------------------------------------------
|   PLT_MicroMediaController::HandleCmd_getmr
+---------------------------------------------------------------------*/
void
PLT_MicroMediaController::getmr(char *uuid, int len)
{
    PLT_DeviceDataReference device;
 
	if(!uuid || len<=0){
		return;
	}

    GetCurMediaRenderer(device);
	
	memset(uuid,0,len);
	
    if (!device.IsNull()) {
        //printf("Current media renderer: %s\n", (const char*)device->GetFriendlyName());
		strncpy(uuid, (const char*)device->GetUUID(),len);
    } 
}


/*----------------------------------------------------------------------
|   PLT_MicroMediaController::HandleCmd_open
+---------------------------------------------------------------------*/
void
PLT_MicroMediaController::open(const char* url, const char* didl)
{
    PLT_DeviceDataReference device;
    GetCurMediaRenderer(device);
    if (!device.IsNull()) {
		// invoke the setUri
		NPT_LOG_INFO_3("Issuing SetAVTransportURI with url=%s , didl=%s", __FUNCTION__,url, didl);
		SetAVTransportURI(device, 0, url, didl, NULL);
    }
}

/*----------------------------------------------------------------------
|   PLT_MicroMediaController::HandleCmd_play
+---------------------------------------------------------------------*/
void
PLT_MicroMediaController::play()
{
    PLT_DeviceDataReference device;
    GetCurMediaRenderer(device);
    if (!device.IsNull()) {
        Play(device, 0, "1", NULL);
    }
}

void 	
PLT_MicroMediaController::pause()
{
    PLT_DeviceDataReference device;
    GetCurMediaRenderer(device);
    if (!device.IsNull()) {
        Pause(device, 0, NULL);
    }
}

/*----------------------------------------------------------------------
|   PLT_MicroMediaController::HandleCmd_seek
+---------------------------------------------------------------------*/
void
PLT_MicroMediaController::seek(const char* command)
{
    PLT_DeviceDataReference device;
    GetCurMediaRenderer(device);
    if (!device.IsNull()) {
        // remove first part of command ("seek")
        NPT_String target = command;
        //NPT_List<NPT_String> args = target.Split(" ");
        //if (args.GetItemCount() < 2) return;

        //args.Erase(args.GetFirstItem());
        //target = NPT_String::Join(args, " ");
        
        //Seek(device, 0, (target.Find(":")!=-1)?"REL_TIME":"X_DLNA_REL_BYTE", target, NULL);
		Seek(device, 0, "REL_TIME", target, NULL);
    }
}

/*----------------------------------------------------------------------
|   PLT_MicroMediaController::HandleCmd_stop
+---------------------------------------------------------------------*/
void
PLT_MicroMediaController::stop()
{
    PLT_DeviceDataReference device;
    GetCurMediaRenderer(device);
    if (!device.IsNull()) {
        Stop(device, 0, NULL);
    }
}

/*----------------------------------------------------------------------
|   PLT_MicroMediaController::HandleCmd_mute
+---------------------------------------------------------------------*/
void
PLT_MicroMediaController::setMute(bool yes)
{
    PLT_DeviceDataReference device;
    GetCurMediaRenderer(device);
    if (!device.IsNull()) {
        SetMute(device, 0, "Master", yes, NULL);
    }
}

/*----------------------------------------------------------------------
|   PLT_MicroMediaController::HandleCmd_unmute
+---------------------------------------------------------------------*/
void
PLT_MicroMediaController::getMute()
{
    PLT_DeviceDataReference device;
    GetCurMediaRenderer(device);
    if (!device.IsNull()) {
        GetMute(device, 0, "Master", NULL);
    }
}

void    
PLT_MicroMediaController::getVolumeRange(int& volMin, int& volMax)
{
	PLT_DeviceDataReference device;
    GetCurMediaRenderer(device);
    if (!device.IsNull()) {
        PLT_Service* service = NULL;
        if (NPT_SUCCEEDED(device->FindServiceByType("urn:schemas-upnp-org:service:RenderingControl:*", service))) {
                if(service){
                        PLT_StateVariable* stval = service->FindStateVariable("Volume");
                        if(stval){
                                const NPT_AllowedValueRange* avr = stval->GetAllowedValueRange();
                                if(avr){
									volMin = avr->min_value;
									volMax = avr->max_value;
									NPT_LOG_INFO_2("[min vol=%d,max vol=%d]",avr->min_value,avr->max_value);
                                }else{
									NPT_LOG_INFO("*** StateVariable Volume AllowedValueRange not found\n");
                                }
                        }else{
							NPT_LOG_INFO("*** StateVariable Volume not found\n");
                        }
                }else{
					NPT_LOG_INFO("*** serice NULL,RenderingControl servie not found\n");
                }
        }else{
			NPT_LOG_INFO("*** RenderingControl servie not found\n");
        }
    }
}


void    
PLT_MicroMediaController::setVolume(int vol)
{
	NPT_LOG_INFO_2("%s into. vol=%d",__FUNCTION__,vol);
	
	PLT_DeviceDataReference device;
    GetCurMediaRenderer(device);
    if (!device.IsNull()) {
        SetVolume(device, 0, "Master", vol,NULL);
    }
}

void    
PLT_MicroMediaController::getVolume()
{
	NPT_LOG_INFO_1("%s into.",__FUNCTION__);
	PLT_DeviceDataReference device;
    GetCurMediaRenderer(device);
    if (!device.IsNull()) {
        GetVolume(device, 0, "Master", NULL);
    }
}


void    
PLT_MicroMediaController::getMediaInfo()
{
	NPT_LOG_INFO_1("%s into.",__FUNCTION__);
	PLT_DeviceDataReference device;
    GetCurMediaRenderer(device);
    if (!device.IsNull()) {
        GetMediaInfo(device, 0, NULL);
    }
}

void    
PLT_MicroMediaController::getPositionInfo()
{
	NPT_LOG_INFO_1("%s into.",__FUNCTION__);
	PLT_DeviceDataReference device;
    GetCurMediaRenderer(device);
    if (!device.IsNull()) {
        GetPositionInfo(device, 0, NULL);
    }
}