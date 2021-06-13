#pragma once
#include "IAgoraRtcEngine.h"

using namespace agora::rtc;

class CAgoraManager;

class CAGEngineEventHandler :
	public IRtcEngineEventHandler
{
public:
	explicit CAGEngineEventHandler(CAgoraManager* manager);
	~CAGEngineEventHandler();
	void SetConnectionId(conn_id_t connId) { conn_id_ = connId; }
	conn_id_t GetConnectionId() { return conn_id_; }
	virtual void onJoinChannelSuccess(const char* channel, uid_t uid, int elapsed);
	virtual void onLeaveChannel(const RtcStats& stat);
	virtual void onUserJoined(uid_t uid, int elapsed);
	virtual void onUserOffline(uid_t uid, USER_OFFLINE_REASON_TYPE reason);
	virtual void onError(int err, const char* msg);
	virtual void onConnectionStateChanged(CONNECTION_STATE_TYPE state, CONNECTION_CHANGED_REASON_TYPE reason);
    virtual void onMediaDeviceChanged(int deviceType);
private:
	conn_id_t conn_id_ = agora::rtc::DUMMY_CONNECTION_ID;
	CAgoraManager* manager_ = nullptr;
};

