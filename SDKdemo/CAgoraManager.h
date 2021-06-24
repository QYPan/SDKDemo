#pragma once
#include "IAgoraRtcEngine.h"
#include "IAgoraMediaEngine.h"

#include <vector>
#include <set>
#include <string>
#include <mutex>

using namespace agora::rtc;

class CAGEngineEventHandler;
class VideoFrameObserver;

class CAgoraManager
{
	CAgoraManager();
	~CAgoraManager();
public:
	struct MicInfo {
		int idx = -1;
		std::string device_id;
		std::string device_name_utf8;
	};

	struct PlaybackInfo {
		int idx = -1;
		std::string device_id;
		std::string device_name_utf8;
	};

	struct CameraInfo {
		int idx = -1;
		std::string device_id;
		std::string device_name_utf8;
	};

	struct AgoraImageBuffer
	{
		std::vector<unsigned char> buffer;     ///< 图内容
		uint32_t length;         ///< 图缓存大小
		uint32_t width;          ///< 图宽
		uint32_t height;         ///< 图高
		AgoraImageBuffer()
			: length(0)
			, width(0)
			, height(0)
		{};
	};

	struct WindowInfo {
		HWND sourceId;
		std::string    sourceName;///< 窗口标题名称，UTF8 编码
		bool           isMinimizeWindow;///< 是否为最小化窗口
		int    x; //窗口位置x
		int    y; //窗口位置y
		int    width;//窗口宽度
		int    height;//窗口高度
		AgoraImageBuffer iconBGRA;           ///< 图标内容
		AgoraImageBuffer thumbBGRA;          ///< 缩略图内容
	};

	struct DesktopInfo
	{
		int sourceId;
		std::string   sourceName;///< 采集源名称，UTF8 编码
		int    x; //全局虚拟桌面位置x
		int    y; //全局虚拟桌面位置y
		int    width;//选定桌面宽度
		int    height;//选定桌面窗口高度
		AgoraImageBuffer thumbBGRA;       ///< 缩略图内容
		bool            isMainScreen;    ///< 是否为主屏
	};

	enum PushSystemAudioOption {
		PushSystemAudioOption_None = 0,
		PushSystemAudioOption_Camera,
		PushSystemAudioOption_Screen,
	};
public:
	//首次调用需写入AppID
	static CAgoraManager* GetInstace();
	static void Destroy();

	bool Init(const char* lpAppID);
	void Release();

	//加入房间
	bool JoinChannel(const char* lpChannelId,
		const char* lpToken = nullptr, agora::rtc::uid_t uid = 0,
		const char* lpSubToken = nullptr, agora::rtc::uid_t uidSub = 0,
		const char* lpSrcToken = nullptr, agora::rtc::uid_t uidSrc = 0);
	//离开房间
	bool LeaveChannel();
	//是否加入房间
	bool IsJoinChannel();


	//推流屏幕相关功能
	bool StartPushScreen(bool bWithMic = false, int nPushFps = 15);
	void UpdatePushScreenConfig(int nPushW, int nPushH, int nPushFrameRate);
	bool StopPushScreen();
	bool IsPushScreen();

	//设置采集窗口(是否可以在推中设置)
	void SetPushWindow(HWND hwnd = 0,
		int x = 0, int y = 0, int w = 0, int h = 0);

	void GetWindowList(std::vector<WindowInfo>& vWindows, int nThumbSizeW, int nThumbSizeH, int nIconSizeW, int nIconSizeH);//获得当前可采集窗口列表及属性

	//设置采集桌面(是否可以在推中设置)
	void SetPushFilter(HWND* pFilterHwndList = nullptr, int nFilterNum = 0);
	void SetPushDesktop(int nScreenID = 0,
		int x = 0, int y = 0, int w = 0, int h = 0);

	void GetDesktopList(std::vector<DesktopInfo>& vDesktop, int nThumbSizeW, int nThumbSizeH);//获得当前可采集桌面列表及属性

	//设置采集图像显示窗口(0为取消指定窗口)
	void SetWindowDesktopShowHwnd(HWND hwnd = 0, agora::media::base::RENDER_MODE_TYPE renderMode = agora::media::base::RENDER_MODE_TYPE::RENDER_MODE_FIT);
	//设置暂停屏幕推流图像
	void SetPushScreenPause(bool bPause = true);
	bool IsPushScreenPause();
	void SetPushScreenAudioMute(bool bMute = true);
	bool IsPushScreenAudioMute();


	//摄像头相关
	bool StartPushCamera(bool bWithMic = true, bool bReverseX = false);
	void UpdatePushCameraConfig(int nPushW, int nPushH, int nPushFrameRate);
	bool StopPushCamera();
	bool IsPushCamera();
	//设置采集摄像头
	void SetPushCamera(int nCamID = -1);//-1为默认摄像头
	void GetCameraList(std::vector<CameraInfo>& vCamera);//获得当前可采集摄像头列表及属性
	//void GetCameraParamList(int nCamID, std::vector<CameraParam>& vParam);//获得当前指定摄像头参数
	//设置摄像头显示窗口(0为取消指定窗口)
	void SetCameraShowHwnd(HWND hwnd = 0, agora::media::base::RENDER_MODE_TYPE renderMode = agora::media::base::RENDER_MODE_TYPE::RENDER_MODE_FIT);
	//获得当前采集尺寸
	void GetCameraSize(int& nRetW, int& nRetH);
	//设置摄像头推流状态
	void SetPushCameraPause(bool bPause = true);
	bool IsPushCameraPause();
	void SetPushCameraAudioMute(bool bMute = true);
	bool IsPushCameraAudioMute();

	void SetPlayerShowHwnd(agora::rtc::uid_t uid, HWND hwnd = 0, agora::media::base::RENDER_MODE_TYPE renderMode = agora::media::base::RENDER_MODE_TYPE::RENDER_MODE_FIT);


	//麦克相关
	void SetMic(int nID = -1);//-1为默认麦克风
	void GetMicList(std::vector<MicInfo>& vMic);//获得当前麦克风列表及属性
	void GetPlaybackList(std::vector<PlaybackInfo>& vPlayback);//获得当前扬声器列表及属性

	// - 0: Mute the recording volume.
    // - 100: The Original volume.
    // - 400: (Maximum) Four times the original volume with signal clipping
	void SetMicVolume(int nVol);

	// The value range is [0, 255].
	void SetSystemMicVolume(int nVol);

	//设置推送系统模式(0.不推送1.与摄像头推送2.与屏幕流推送),参考PushSystemAudioOption,该函数只能在加入频道之后调用,因为只有加入频道后才能拿到connection id
	void SetPushSystemAudio(int nMode);


	//播放指定远端uid对象
	void StartPlayer(agora::rtc::uid_t uid);
	//停止播放指定远端uid对象
	void StopPlayer(agora::rtc::uid_t uid);
	//指定远端uid对象设置画面暂停
	void PausePlayer(agora::rtc::uid_t uid, bool bPause = true);
	//指定远端uid对象设置静音状态
	void MutePlayer(agora::rtc::uid_t uid, bool bMute = true);

	//开始自定义推流
	bool StartPushCustom();
	//停止自定义推流
	bool StopPushCustom();
	//是否自定义推流
	bool IsPushCustom();
	//推送视频帧(BGRA)
	bool PushVideoFrame(unsigned char* pData, int nW, int nH, long long ms);
	//推送音频帧(PCM-16)
	bool PushAudioFrame(unsigned char* pData, int nbSamples, 
		long lSampleRate = 48000, int nChannel = 2, long long ms = 0);


	//获得图像数据RGBA
	//获得指定远端uid对象播放的图像数据
	bool GetPlayerImageSize(agora::rtc::uid_t uid, int& nRetW, int& nRetH);
	bool GetPlayerImage(agora::rtc::uid_t uid, BYTE* pData, int& nRetW, int& nRetH);
	//获得推流摄像头的图像数据
	bool GetCameraImageSize(int& nRetW, int& nRetH);
	bool GetCameraImage(BYTE* pData, int& nRetW, int& nRetH);
	//获得推流屏幕的图像数据
	bool GetScreenImageSize(int& nRetW, int& nRetH);
	bool GetScreenImage(BYTE* pData, int& nRetW, int& nRetH);
	//获得当前所有可播放的uid
	void GetPlayerUID(std::vector<agora::rtc::uid_t>& uidList);

	int StartPreview();
	int StopPreview();

	void EnableVideoFrameObserver(bool enable);

	// Rtc event
	// 加入房间成功
	void OnJoinChannelSuccess(const char* channel, uid_t uid, int elapsed, conn_id_t connId);
	void OnLeaveChannel(const RtcStats& stat, conn_id_t connId);
	// 远端用户加入
	void OnUserJoined(uid_t uid, int elapsed, conn_id_t connId);
	// 远端用户离开
	void OnUserOffline(uid_t uid, USER_OFFLINE_REASON_TYPE reason, conn_id_t connId);
	// 错误回调(不推荐使用，由以下回调单独提供错误信息)
	// onConnectionStateChanged
	// onMediaDeviceChanged
	// onLocalVideoStateChanged
	// onLocalAudioStateChanged
	// onRemoteVideoStateChanged
	// onRemoteAudioStateChanged
	// onFirstLocalVideoFramePublished
	// onFirstLocalAudioFramePublished
	void onError(int err, const char* msg, conn_id_t connId);
	// 网络状态回调
	void onConnectionStateChanged(CONNECTION_STATE_TYPE state, CONNECTION_CHANGED_REASON_TYPE reason, conn_id_t connId);
	// 设备变化回调
	void onMediaDeviceChanged(int deviceType, conn_id_t connId);
	// 本端视频状态回调
	void onLocalVideoStateChanged(LOCAL_VIDEO_STREAM_STATE state, LOCAL_VIDEO_STREAM_ERROR error, conn_id_t connId);
	// 本端音频状态回调
	void onLocalAudioStateChanged(LOCAL_AUDIO_STREAM_STATE state, LOCAL_AUDIO_STREAM_ERROR error, conn_id_t connId);
	// 远端视频状态回调
	void onRemoteVideoStateChanged(uid_t uid, REMOTE_VIDEO_STATE state, REMOTE_VIDEO_STATE_REASON reason, int elapsed, conn_id_t connId);
	// 远端音频状态回调
	void onRemoteAudioStateChanged(uid_t uid, REMOTE_AUDIO_STATE state, REMOTE_AUDIO_STATE_REASON reason, int elapsed, conn_id_t connId);
	// 视频推流成功回调
	void onFirstLocalVideoFramePublished(int elapsed, conn_id_t connId);
	// 音频推流成功回调
	void onFirstLocalAudioFramePublished(int elapsed, conn_id_t connId);

	// 具体设备插拔回调
	// deviceType - 设备类型 #MEDIA_DEVICE_TYPE
	// deviceId   - 设备 id
	// state      - 设备状态 0: 移除 1: 插入
	void onMediaDeviceStateChanged(int deviceType, std::string deviceId, int state);

private:
	void ResetStates();

	static CAgoraManager* instance_;
	IRtcEngine* rtc_engine_ = nullptr;
	agora::media::IMediaEngine* media_engine_ = nullptr;
	CAGEngineEventHandler* camera_event_handler_ = nullptr;
	CAGEngineEventHandler* screen_event_handler_ = nullptr;
	CAGEngineEventHandler* custom_event_handler_ = nullptr;
	VideoFrameObserver* video_frame_observer_ = nullptr;

	agora::rtc::Rectangle screen_rect_;
	agora::rtc::Rectangle region_rect_;

	std::set<uid_t> users_in_channel_;
	std::vector<agora::view_t> exclude_window_list_;

	ScreenCaptureParameters param_;

	bool is_joined_ = false;
	bool is_publish_camera_ = false;
	bool is_publish_screen_ = false;
	bool is_publish_camera_audio_ = false;
	bool is_publish_screen_audio_ = false;
	bool is_mute_camera_ = false;
	bool is_mute_screen_ = false;
	bool is_mute_mic_ = false;
	bool is_preview_ = false;
	bool is_enable_video_observer_ = false;

	bool is_publish_custom_ = false;

	int push_screen_width_ = 0;
	int push_screen_height_ = 0;

	uid_t camera_uid_ = 0;
	uid_t screen_uid_ = 0;
	uid_t custom_uid_ = 0;

	HWND share_win_ = 0;

	agora::view_t camera_view_ = nullptr;
	agora::view_t screen_view_ = nullptr;
	agora::view_t custom_view_ = nullptr;

	std::vector<CameraInfo> camera_list_;
	std::vector<MicInfo> mic_list_;
	std::vector<PlaybackInfo> playback_list_;

	std::unique_ptr<agora::rtc::IVideoDeviceManager> vdm_;
	std::unique_ptr<agora::rtc::IAudioDeviceManager> adm_;

	conn_id_t camera_connId_ = agora::rtc::DUMMY_CONNECTION_ID;
	conn_id_t screen_connId_ = agora::rtc::DUMMY_CONNECTION_ID;
	conn_id_t custom_connId_ = agora::rtc::DUMMY_CONNECTION_ID;

	int current_recording_mode_ = 0;

	CameraInfo current_camera_ = CameraInfo{0, "", ""};
	bool initialized_ = false;

	std::mutex mtx_;
};
