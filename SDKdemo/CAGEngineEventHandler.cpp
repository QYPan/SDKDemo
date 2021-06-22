#include "stdafx.h"
#include "CAGEngineEventHandler.h"
#include "CAgoraManager.h"


CAGEngineEventHandler::CAGEngineEventHandler(CAgoraManager* manager)
	: manager_(manager)
{

}


CAGEngineEventHandler::~CAGEngineEventHandler()
{
}

void CAGEngineEventHandler::onJoinChannelSuccess(const char* channel, uid_t uid, int elapsed)
{
	if (manager_) {
		manager_->OnJoinChannelSuccess(channel, uid, elapsed, conn_id_);
	}
}

void CAGEngineEventHandler::onLeaveChannel(const RtcStats& stat)
{
	if (manager_) {
		manager_->OnLeaveChannel(stat, conn_id_);
	}
}

void CAGEngineEventHandler::onUserJoined(uid_t uid, int elapsed)
{
	if (manager_) {
		manager_->OnUserJoined(uid, elapsed, conn_id_);
	}
}

void CAGEngineEventHandler::onUserOffline(uid_t uid, USER_OFFLINE_REASON_TYPE reason)
{
	if (manager_) {
		manager_->OnUserOffline(uid, reason, conn_id_);
	}
}

void CAGEngineEventHandler::onError(int err, const char* msg) {
	if (manager_) {
		manager_->onError(err, msg, conn_id_);
	}
}

void CAGEngineEventHandler::onConnectionStateChanged(CONNECTION_STATE_TYPE state, CONNECTION_CHANGED_REASON_TYPE reason) {
	if (manager_) {
		manager_->onConnectionStateChanged(state, reason, conn_id_);
	}
}

void CAGEngineEventHandler::onMediaDeviceChanged(int deviceType) { 
	if (manager_) {
		manager_->onMediaDeviceChanged(deviceType, conn_id_);
	}
}

void CAGEngineEventHandler::onLocalVideoStateChanged(LOCAL_VIDEO_STREAM_STATE state, LOCAL_VIDEO_STREAM_ERROR error) {
	if (manager_) {
		manager_->onLocalVideoStateChanged(state, error, conn_id_);
	}
}

void CAGEngineEventHandler::onLocalAudioStateChanged(LOCAL_AUDIO_STREAM_STATE state, LOCAL_AUDIO_STREAM_ERROR error) {
	if (manager_) {
		manager_->onLocalAudioStateChanged(state, error, conn_id_);
	}
}

void CAGEngineEventHandler::onRemoteVideoStateChanged(uid_t uid, REMOTE_VIDEO_STATE state, REMOTE_VIDEO_STATE_REASON reason, int elapsed) {
	if (manager_) {
		manager_->onRemoteVideoStateChanged(uid, state, reason, elapsed, conn_id_);
	}
}

void CAGEngineEventHandler::onRemoteAudioStateChanged(uid_t uid, REMOTE_AUDIO_STATE state, REMOTE_AUDIO_STATE_REASON reason, int elapsed) {
	if (manager_) {
		manager_->onRemoteAudioStateChanged(uid, state, reason, elapsed, conn_id_);
	}
}

void CAGEngineEventHandler::onFirstLocalVideoFramePublished(int elapsed) {
	if (manager_) {
		manager_->onFirstLocalVideoFramePublished(elapsed, conn_id_);
	}
}

void CAGEngineEventHandler::onFirstLocalAudioFramePublished(int elapsed) {
	if (manager_) {
		manager_->onFirstLocalAudioFramePublished(elapsed, conn_id_);
	}
}