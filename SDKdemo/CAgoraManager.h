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
	//�״ε�����д��AppID
	static CAgoraManager* Inst();
	static void Destroy();

	bool Init(const char* lpAppID);
	void Release();

	//���뷿��
	bool JoinChannel(const char* lpChannelId,
		const char* lpToken = nullptr, agora::rtc::uid_t uid = 0,
		const char* lpSubToken = nullptr, agora::rtc::uid_t subuid = 0);
	//�뿪����
	bool LeaveChannel();
	//�Ƿ���뷿��
	bool IsJoinChannel();


	//������Ļ��ع���
	bool StartPushScreen(bool bWithMic = false, int nPushW = 0, int nPushH = 0);
	bool StopPushScreen();
	bool IsPushScreen();

	//���òɼ�����(�Ƿ��������������)
	void SetPushWindow(HWND hwnd = 0,
		int x = 0, int y = 0, int w = 0, int h = 0);
	//void GetWindowList(std::vector<WinProg>& vWindows);//��õ�ǰ�ɲɼ������б�����
	//���òɼ�����(�Ƿ��������������)
	void SetPushFilter(HWND* pFilterHwndList = nullptr, int nFilterNum = 0);
	void SetPushDesktop(int nScreenID = 0,
		int x = 0, int y = 0, int w = 0, int h = 0);
	//void GetDesktopList(std::vector<DesktopProg>& vDesktop);//��õ�ǰ�ɲɼ������б�����
	//��õ�ǰ�ɼ�����/����ߴ�
	void GetWindowDesktopSize(int& nRetW, int& nRetH);
	//���òɼ�ͼ����ʾ����(0Ϊȡ��ָ������)
	void SetWindowDesktopShowHwnd(HWND hwnd = 0);
	//������ͣ��Ļ����ͼ��
	void SetScreenPushPause(bool bPause = true);
	bool IsScreenPushPause();
	void SetPushScreenMicMute(bool bMute = true);
	bool IsPushScreenMicMute();


	//����ͷ���
	bool StartPushCamera(bool bWithMic = true, int nPushW = 0, int nPushH = 0);
	bool StopPushCamera();
	bool IsPushCamera();
	//���òɼ�����ͷ
	void SetPushCamera(int nCamID = -1, int nW = 0, int nH = 0);//-1ΪĬ������ͷ
	void GetCameraList(std::vector<CameraProg>& vCamera);//��õ�ǰ�ɲɼ�����ͷ�б�����
	//void GetCameraParamList(int nCamID, std::vector<CameraParam>& vParam);//��õ�ǰָ������ͷ����
	//��������ͷ��ʾ����(0Ϊȡ��ָ������)
	void SetCameraShowHwnd(HWND hwnd = 0);
	//��õ�ǰ�ɼ��ߴ�
	void GetCameraSize(int& nRetW, int& nRetH);
	//��������ͷ����״̬
	void SetPushCameraPause(bool bPause = true);
	bool IsPushCameraPause();
	void SetPushCameraMicMute(bool bMute = true);
	bool IsPushCameraMicMute();

	void SetPlayerShowHwnd(agora::rtc::uid_t uid, HWND hwnd = 0);


	//������
	void SetMic(int nID = -1);//-1ΪĬ����˷�
	void GetMicList(std::vector<MicProg>& vMic);//��õ�ǰ��˷��б�����
	void SetMicVolume(int nVol);


	//����ָ��Զ��uid����
	void PlayVideo(agora::rtc::uid_t uid);
	//ֹͣ����ָ��Զ��uid����
	void StopVideo(agora::rtc::uid_t uid);
	//ָ��Զ��uid�������þ���״̬
	void MuteVideo(agora::rtc::uid_t uid, bool bMute = true);


	//���ͼ������RGBA
	//���ָ��Զ��uid���󲥷ŵ�ͼ������
	int GetPlayerImageW(agora::rtc::uid_t uid);
	int GetPlayerImageH(agora::rtc::uid_t uid);
	bool GetPlayerImage(agora::rtc::uid_t uid, BYTE* pData, int& nRetW, int& nRetH);
	//�����������ͷ��ͼ������
	int GetCameraImageW();
	int GetCameraImageH();
	bool GetCameraImage(BYTE* pData, int& nRetW, int& nRetH);
	//���������Ļ��ͼ������
	int GetScreenImageW();
	int GetScreenImageH();
	bool GetScreenImage(BYTE* pData, int& nRetW, int& nRetH);
	//��õ�ǰ���пɲ��ŵ�uid
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
