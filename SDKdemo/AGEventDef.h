#pragma once
#include <string>

#define MSGID_JOIN_CHANNEL_SUCCESS				WM_USER+0x1000
#define MSGID_LEAVE_CHANNEL				        WM_USER+0x1001
#define MSGID_USER_JOINED			        	WM_USER+0x1002
#define MSGID_USER_OFFLINE				        WM_USER+0x1003

struct JoinChannelSuccessData
{
	std::string channelName;
	unsigned int uid;
	int elapsed;
};

struct LeaveChannelData
{
	unsigned int txBytes;
	unsigned int rxBytes;
	unsigned short txKBitRate;
	unsigned short rxKBitRate;
};

struct UserJoinedData
{
	unsigned int uid;
	int elapsed;
};

struct UserOfflineData
{
	unsigned int uid;
	int reason;
};