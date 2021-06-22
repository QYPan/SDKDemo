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
	virtual void onLocalVideoStateChanged(LOCAL_VIDEO_STREAM_STATE state, LOCAL_VIDEO_STREAM_ERROR error);
	virtual void onLocalAudioStateChanged(LOCAL_AUDIO_STREAM_STATE state, LOCAL_AUDIO_STREAM_ERROR error);
	virtual void onRemoteVideoStateChanged(uid_t uid, REMOTE_VIDEO_STATE state, REMOTE_VIDEO_STATE_REASON reason, int elapsed);
	virtual void onRemoteAudioStateChanged(uid_t uid, REMOTE_AUDIO_STATE state, REMOTE_AUDIO_STATE_REASON reason, int elapsed);
	virtual void onFirstLocalVideoFramePublished(int elapsed);
	virtual void onFirstLocalAudioFramePublished(int elapsed);

private:
	conn_id_t conn_id_ = agora::rtc::DUMMY_CONNECTION_ID;
	CAgoraManager* manager_ = nullptr;
};

