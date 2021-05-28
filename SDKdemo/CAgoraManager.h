#pragma once
#include "IAgoraRtcEngine.h"
#include "CAGEngineEventHandler.h"
#include "IAgoraMediaEngine.h"

#include <vector>
#include <set>
#include <string>

using namespace agora::rtc;

class CAGEngineEventHandler;
class VideoFrameObserver;

class CAgoraManager
{
	CAgoraManager();
	~CAgoraManager();
public:
	struct MicProg {
		int idx = -1;
		std::string device_id;
		std::string device_name_utf8;
	};
	struct CameraProg {
		int idx = -1;
		std::string device_id;
		std::string device_name_utf8;
	};
public:
	//首次调用需写入AppID
	static CAgoraManager* Inst();
	static void Destroy();

	bool Init(const char* lpAppID);
	void Release();

	//加入房间
	bool JoinChannel(const char* lpChannelId,
		const char* lpToken = nullptr, agora::rtc::uid_t uid = 0,
		const char* lpSubToken = nullptr, agora::rtc::uid_t subuid = 0);
	//离开房间
	bool LeaveChannel();
	//是否加入房间
	bool IsJoinChannel();


	//推流屏幕相关功能
	bool StartPushScreen(bool bWithMic = false, int nPushW = 0, int nPushH = 0);
	bool StopPushScreen();
	bool IsPushScreen();

	//设置采集窗口(是否可以在推中设置)
	void SetPushWindow(HWND hwnd = 0,
		int x = 0, int y = 0, int w = 0, int h = 0);
	//void GetWindowList(std::vector<WinProg>& vWindows);//获得当前可采集窗口列表及属性
	//设置采集桌面(是否可以在推中设置)
	void SetPushFilter(HWND* pFilterHwndList = nullptr, int nFilterNum = 0);
	void SetPushDesktop(int nScreenID = 0,
		int x = 0, int y = 0, int w = 0, int h = 0);
	//void GetDesktopList(std::vector<DesktopProg>& vDesktop);//获得当前可采集桌面列表及属性
	//获得当前采集窗口/桌面尺寸
	void GetWindowDesktopSize(int& nRetW, int& nRetH);
	//设置采集图像显示窗口(0为取消指定窗口)
	void SetWindowDesktopShowHwnd(HWND hwnd = 0);
	//设置暂停屏幕推流图像
	void SetScreenPushPause(bool bPause = true);
	bool IsScreenPushPause();
	void SetPushScreenMicMute(bool bMute = true);
	bool IsPushScreenMicMute();


	//摄像头相关
	bool StartPushCamera(bool bWithMic = true, int nPushW = 0, int nPushH = 0);
	bool StopPushCamera();
	bool IsPushCamera();
	//设置采集摄像头
	void SetPushCamera(int nCamID = -1, int nW = 0, int nH = 0);//-1为默认摄像头
	void GetCameraList(std::vector<CameraProg>& vCamera);//获得当前可采集摄像头列表及属性
	//void GetCameraParamList(int nCamID, std::vector<CameraParam>& vParam);//获得当前指定摄像头参数
	//设置摄像头显示窗口(0为取消指定窗口)
	void SetCameraShowHwnd(HWND hwnd = 0);
	//获得当前采集尺寸
	void GetCameraSize(int& nRetW, int& nRetH);
	//设置摄像头推流状态
	void SetPushCameraPause(bool bPause = true);
	bool IsPushCameraPause();
	void SetPushCameraMicMute(bool bMute = true);
	bool IsPushCameraMicMute();

	void SetPlayerShowHwnd(agora::rtc::uid_t uid, HWND hwnd = 0);


	//麦克相关
	void SetMic(int nID = -1);//-1为默认麦克风
	void GetMicList(std::vector<MicProg>& vMic);//获得当前麦克风列表及属性
	void SetMicVolume(int nVol);


	//播放指定远端uid对象
	void PlayVideo(agora::rtc::uid_t uid);
	//停止播放指定远端uid对象
	void StopVideo(agora::rtc::uid_t uid);
	//指定远端uid对象设置静音状态
	void MuteVideo(agora::rtc::uid_t uid, bool bMute = true);


	//获得图像数据RGBA
	//获得指定远端uid对象播放的图像数据
	int GetPlayerImageW(agora::rtc::uid_t uid);
	int GetPlayerImageH(agora::rtc::uid_t uid);
	bool GetPlayerImage(agora::rtc::uid_t uid, BYTE* pData, int& nRetW, int& nRetH);
	//获得推流摄像头的图像数据
	int GetCameraImageW();
	int GetCameraImageH();
	bool GetCameraImage(BYTE* pData, int& nRetW, int& nRetH);
	//获得推流屏幕的图像数据
	int GetScreenImageW();
	int GetScreenImageH();
	bool GetScreenImage(BYTE* pData, int& nRetW, int& nRetH);
	//获得当前所有可播放的uid
	void GetPlayerUID(std::vector<agora::rtc::uid_t>& uidList);

	int StartPreview();
	int StopPreview();

	void EnableVideoFrameObserver(bool enable);
	agora::util::AutoPtr<agora::media::IMediaEngine>& GetMediaEngine();

	// Rtc event
	void OnJoinChannelSuccess(const char* channel, uid_t uid, int elapsed, conn_id_t connId);
	void OnLeaveChannel(const RtcStats& stat, conn_id_t connId);
	void OnUserJoined(uid_t uid, int elapsed, conn_id_t connId);
	void OnUserOffline(uid_t uid, USER_OFFLINE_REASON_TYPE reason, conn_id_t connId);
	void onError(int err, const char* msg, conn_id_t connId);

private:
	void RestStates();

	static CAgoraManager* instance_;
	IRtcEngine* rtc_engine_ = nullptr;
	CAGEngineEventHandler* camera_event_handler_ = nullptr;
	CAGEngineEventHandler* screen_event_handler_ = nullptr;
	VideoFrameObserver* video_frame_observer_ = nullptr;

	agora::rtc::Rectangle region_rect_;

	agora::util::AutoPtr<agora::media::IMediaEngine> media_engine_;

	std::set<uid_t> users_in_channel_;
	std::vector<agora::view_t> exclude_window_list_;

	bool is_joined_ = false;
	bool is_publish_camera_ = false;
	bool is_publish_screen_ = false;
	bool is_mute_camera_ = false;
	bool is_mute_screen_ = false;
	bool is_mute_mic_ = false;
	bool is_preview_ = false;
	bool is_enable_video_observer_ = false;

	int push_screen_width_ = 0;
	int push_screen_height_ = 0;

	uid_t camera_uid_ = 0;
	uid_t screen_uid_ = 0;

	HWND share_win_ = 0;

	agora::view_t camera_view_ = nullptr;
	agora::view_t screen_view_ = nullptr;

	conn_id_t camera_connId_ = INVALID_CONNECTION_ID;
	conn_id_t screen_connId_ = INVALID_CONNECTION_ID;

	bool initialized_ = false;
};
