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

}
