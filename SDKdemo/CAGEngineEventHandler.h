#pragma once
#include "IAgoraRtcEngine.h"

using namespace agora::rtc;

class CAGEngineEventHandler :
	public IRtcEngineEventHandler
{
public:
	CAGEngineEventHandler();
	~CAGEngineEventHandler();
	void setMainWnd(HWND wnd);
	virtual void onJoinChannelSuccess(const char* channel, uid_t uid, int elapsed);
	virtual void onLeaveChannel(const RtcStats& stat);
	virtual void onUserJoined(uid_t uid, int elapsed);
	virtual void onUserOffline(uid_t uid, USER_OFFLINE_REASON_TYPE reason);
private:
	HWND m_hMainWnd;
};

