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
		std::vector<unsigned char> buffer;     ///< ͼ����
		uint32_t length;         ///< ͼ�����С
		uint32_t width;          ///< ͼ��
		uint32_t height;         ///< ͼ��
		AgoraImageBuffer()
			: length(0)
			, width(0)
			, height(0)
		{};
	};

	struct WindowInfo {
		HWND sourceId;
		std::string    sourceName;///< ���ڱ������ƣ�UTF8 ����
		bool           isMinimizeWindow;///< �Ƿ�Ϊ��С������
		int    x; //����λ��x
		int    y; //����λ��y
		int    width;//���ڿ��
		int    height;//���ڸ߶�
		AgoraImageBuffer iconBGRA;           ///< ͼ������
		AgoraImageBuffer thumbBGRA;          ///< ����ͼ����
	};

	struct DesktopInfo
	{
		int sourceId;
		std::string   sourceName;///< �ɼ�Դ���ƣ�UTF8 ����
		int    x; //ȫ����������λ��x
		int    y; //ȫ����������λ��y
		int    width;//ѡ��������
		int    height;//ѡ�����洰�ڸ߶�
		AgoraImageBuffer thumbBGRA;       ///< ����ͼ����
		bool            isMainScreen;    ///< �Ƿ�Ϊ����
	};

	enum PushSystemAudioOption {
		PushSystemAudioOption_None = 0,
		PushSystemAudioOption_Camera,
		PushSystemAudioOption_Screen,
	};
public:
	//�״ε�����д��AppID
	static CAgoraManager* GetInstace();
	static void Destroy();

	bool Init(const char* lpAppID);
	void Release();

	//���뷿��
	bool JoinChannel(const char* lpChannelId,
		const char* lpToken = nullptr, agora::rtc::uid_t uid = 0,
		const char* lpSubToken = nullptr, agora::rtc::uid_t uidSub = 0,
		const char* lpSrcToken = nullptr, agora::rtc::uid_t uidSrc = 0);
	//�뿪����
	bool LeaveChannel();
	//�Ƿ���뷿��
	bool IsJoinChannel();


	//������Ļ��ع���
	bool StartPushScreen(bool bWithMic = false, int nPushFps = 15);
	void UpdatePushScreenConfig(int nPushW, int nPushH, int nPushFrameRate);
	bool StopPushScreen();
	bool IsPushScreen();

	//���òɼ�����(�Ƿ��������������)
	void SetPushWindow(HWND hwnd = 0,
		int x = 0, int y = 0, int w = 0, int h = 0);

	void GetWindowList(std::vector<WindowInfo>& vWindows, int nThumbSizeW, int nThumbSizeH, int nIconSizeW, int nIconSizeH);//��õ�ǰ�ɲɼ������б�����

	//���òɼ�����(�Ƿ��������������)
	void SetPushFilter(HWND* pFilterHwndList = nullptr, int nFilterNum = 0);
	void SetPushDesktop(int nScreenID = 0,
		int x = 0, int y = 0, int w = 0, int h = 0);

	void GetDesktopList(std::vector<DesktopInfo>& vDesktop, int nThumbSizeW, int nThumbSizeH);//��õ�ǰ�ɲɼ������б�����

	//���òɼ�ͼ����ʾ����(0Ϊȡ��ָ������)
	void SetWindowDesktopShowHwnd(HWND hwnd = 0, agora::media::base::RENDER_MODE_TYPE renderMode = agora::media::base::RENDER_MODE_TYPE::RENDER_MODE_FIT);
	//������ͣ��Ļ����ͼ��
	void SetPushScreenPause(bool bPause = true);
	bool IsPushScreenPause();
	void SetPushScreenAudioMute(bool bMute = true);
	bool IsPushScreenAudioMute();


	//����ͷ���
	bool StartPushCamera(bool bWithMic = true, bool bReverseX = false);
	void UpdatePushCameraConfig(int nPushW, int nPushH, int nPushFrameRate);
	bool StopPushCamera();
	bool IsPushCamera();
	//���òɼ�����ͷ
	void SetPushCamera(int nCamID = -1);//-1ΪĬ������ͷ
	void GetCameraList(std::vector<CameraInfo>& vCamera);//��õ�ǰ�ɲɼ�����ͷ�б�����
	//void GetCameraParamList(int nCamID, std::vector<CameraParam>& vParam);//��õ�ǰָ������ͷ����
	//��������ͷ��ʾ����(0Ϊȡ��ָ������)
	void SetCameraShowHwnd(HWND hwnd = 0, agora::media::base::RENDER_MODE_TYPE renderMode = agora::media::base::RENDER_MODE_TYPE::RENDER_MODE_FIT);
	//��õ�ǰ�ɼ��ߴ�
	void GetCameraSize(int& nRetW, int& nRetH);
	//��������ͷ����״̬
	void SetPushCameraPause(bool bPause = true);
	bool IsPushCameraPause();
	void SetPushCameraAudioMute(bool bMute = true);
	bool IsPushCameraAudioMute();

	void SetPlayerShowHwnd(agora::rtc::uid_t uid, HWND hwnd = 0, agora::media::base::RENDER_MODE_TYPE renderMode = agora::media::base::RENDER_MODE_TYPE::RENDER_MODE_FIT);


	//������
	void SetMic(int nID = -1);//-1ΪĬ����˷�
	void GetMicList(std::vector<MicInfo>& vMic);//��õ�ǰ��˷��б�����
	void GetPlaybackList(std::vector<PlaybackInfo>& vPlayback);//��õ�ǰ�������б�����

	// - 0: Mute the recording volume.
    // - 100: The Original volume.
    // - 400: (Maximum) Four times the original volume with signal clipping
	void SetMicVolume(int nVol);

	// The value range is [0, 255].
	void SetSystemMicVolume(int nVol);

	//��������ϵͳģʽ(0.������1.������ͷ����2.����Ļ������),�ο�PushSystemAudioOption,�ú���ֻ���ڼ���Ƶ��֮�����,��Ϊֻ�м���Ƶ��������õ�connection id
	void SetPushSystemAudio(int nMode);


	//����ָ��Զ��uid����
	void StartPlayer(agora::rtc::uid_t uid);
	//ֹͣ����ָ��Զ��uid����
	void StopPlayer(agora::rtc::uid_t uid);
	//ָ��Զ��uid�������û�����ͣ
	void PausePlayer(agora::rtc::uid_t uid, bool bPause = true);
	//ָ��Զ��uid�������þ���״̬
	void MutePlayer(agora::rtc::uid_t uid, bool bMute = true);

	//��ʼ�Զ�������
	bool StartPushCustom();
	//ֹͣ�Զ�������
	bool StopPushCustom();
	//�Ƿ��Զ�������
	bool IsPushCustom();
	//������Ƶ֡(BGRA)
	bool PushVideoFrame(unsigned char* pData, int nW, int nH, long long ms);
	//������Ƶ֡(PCM-16)
	bool PushAudioFrame(unsigned char* pData, int nbSamples, 
		long lSampleRate = 48000, int nChannel = 2, long long ms = 0);


	//���ͼ������RGBA
	//���ָ��Զ��uid���󲥷ŵ�ͼ������
	bool GetPlayerImageSize(agora::rtc::uid_t uid, int& nRetW, int& nRetH);
	bool GetPlayerImage(agora::rtc::uid_t uid, BYTE* pData, int& nRetW, int& nRetH);
	//�����������ͷ��ͼ������
	bool GetCameraImageSize(int& nRetW, int& nRetH);
	bool GetCameraImage(BYTE* pData, int& nRetW, int& nRetH);
	//���������Ļ��ͼ������
	bool GetScreenImageSize(int& nRetW, int& nRetH);
	bool GetScreenImage(BYTE* pData, int& nRetW, int& nRetH);
	//��õ�ǰ���пɲ��ŵ�uid
	void GetPlayerUID(std::vector<agora::rtc::uid_t>& uidList);

	int StartPreview();
	int StopPreview();

	void EnableVideoFrameObserver(bool enable);

	// Rtc event
	// ���뷿��ɹ�
	void OnJoinChannelSuccess(const char* channel, uid_t uid, int elapsed, conn_id_t connId);
	void OnLeaveChannel(const RtcStats& stat, conn_id_t connId);
	// Զ���û�����
	void OnUserJoined(uid_t uid, int elapsed, conn_id_t connId);
	// Զ���û��뿪
	void OnUserOffline(uid_t uid, USER_OFFLINE_REASON_TYPE reason, conn_id_t connId);
	// ����ص�(���Ƽ�ʹ�ã������»ص������ṩ������Ϣ)
	// onConnectionStateChanged
	// onMediaDeviceChanged
	// onLocalVideoStateChanged
	// onLocalAudioStateChanged
	// onRemoteVideoStateChanged
	// onRemoteAudioStateChanged
	// onFirstLocalVideoFramePublished
	// onFirstLocalAudioFramePublished
	void onError(int err, const char* msg, conn_id_t connId);
	// ����״̬�ص�
	void onConnectionStateChanged(CONNECTION_STATE_TYPE state, CONNECTION_CHANGED_REASON_TYPE reason, conn_id_t connId);
	// �豸�仯�ص�
	void onMediaDeviceChanged(int deviceType, conn_id_t connId);
	// ������Ƶ״̬�ص�
	void onLocalVideoStateChanged(LOCAL_VIDEO_STREAM_STATE state, LOCAL_VIDEO_STREAM_ERROR error, conn_id_t connId);
	// ������Ƶ״̬�ص�
	void onLocalAudioStateChanged(LOCAL_AUDIO_STREAM_STATE state, LOCAL_AUDIO_STREAM_ERROR error, conn_id_t connId);
	// Զ����Ƶ״̬�ص�
	void onRemoteVideoStateChanged(uid_t uid, REMOTE_VIDEO_STATE state, REMOTE_VIDEO_STATE_REASON reason, int elapsed, conn_id_t connId);
	// Զ����Ƶ״̬�ص�
	void onRemoteAudioStateChanged(uid_t uid, REMOTE_AUDIO_STATE state, REMOTE_AUDIO_STATE_REASON reason, int elapsed, conn_id_t connId);
	// ��Ƶ�����ɹ��ص�
	void onFirstLocalVideoFramePublished(int elapsed, conn_id_t connId);
	// ��Ƶ�����ɹ��ص�
	void onFirstLocalAudioFramePublished(int elapsed, conn_id_t connId);

	// �����豸��λص�
	// deviceType - �豸���� #MEDIA_DEVICE_TYPE
	// deviceId   - �豸 id
	// state      - �豸״̬ 0: �Ƴ� 1: ����
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
