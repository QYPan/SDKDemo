#include "stdafx.h"
#include "CAGEngineEventHandler.h"
#include "AGEventDef.h"


CAGEngineEventHandler::CAGEngineEventHandler()
	: m_hMainWnd(NULL)
{
}


CAGEngineEventHandler::~CAGEngineEventHandler()
{
}

void CAGEngineEventHandler::setMainWnd(HWND wnd)
{
	m_hMainWnd = wnd;
}

void CAGEngineEventHandler::onJoinChannelSuccess(const char* channel, uid_t uid, int elapsed)
{
	JoinChannelSuccessData data;
	data.channelName = std::string(channel);
	data.uid = uid;
	data.elapsed = elapsed;

	if(m_hMainWnd)
	{
		::SendMessage(m_hMainWnd, MSGID_JOIN_CHANNEL_SUCCESS, reinterpret_cast<WPARAM>(&data), 0);
	}
}

void CAGEngineEventHandler::onLeaveChannel(const RtcStats& stat)
{
	LeaveChannelData data;
	data.txBytes = stat.txBytes;
	data.rxBytes = stat.rxBytes;
	data.txKBitRate = stat.txKBitRate;
	data.rxKBitRate = stat.rxKBitRate;

	if (m_hMainWnd)
	{
		::SendMessage(m_hMainWnd, MSGID_LEAVE_CHANNEL, reinterpret_cast<WPARAM>(&data), 0);
	}
}

void CAGEngineEventHandler::onUserJoined(uid_t uid, int elapsed)
{
	UserJoinedData data;
	data.uid = uid;
	data.elapsed = elapsed;

	if (m_hMainWnd)
	{
		::SendMessage(m_hMainWnd, MSGID_USER_JOINED, reinterpret_cast<WPARAM>(&data), 0);
	}
}

void CAGEngineEventHandler::onUserOffline(uid_t uid, USER_OFFLINE_REASON_TYPE reason)
{
	UserOfflineData data;
	data.uid = uid;
	data.reason = reason;

	if (m_hMainWnd)
	{
		::SendMessage(m_hMainWnd, MSGID_USER_OFFLINE, reinterpret_cast<WPARAM>(&data), 0);
	}
}
