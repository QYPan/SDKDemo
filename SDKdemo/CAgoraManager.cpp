#include "stdafx.h"
#include "CAgoraManager.h"
#include "VideoFrameObserver.h"
#include "CAGEngineEventHandler.h"
#include "WinEnumerImpl.h"

#define RETURN_FALSE_IF_ENGINE_NOT_INITIALIZED() \
  if (!initialized_) {                           \
    return false;                                \
  }

#define RETURN_IF_ENGINE_NOT_INITIALIZED() \
  if (!initialized_) {                     \
    return;                                \
  }

#define RETURN_ERR_IF_ENGINE_NOT_INITIALIZED() \
  if (!initialized_) {                         \
    return -7;                                 \
  }

CAgoraManager* CAgoraManager::instance_ = nullptr;

CAgoraManager::CAgoraManager() {}

CAgoraManager::~CAgoraManager() { Release(); }

CAgoraManager* CAgoraManager::GetInstace() {
	if (instance_ == nullptr) {
		instance_ = new CAgoraManager();
	}
	return instance_;
}

void CAgoraManager::Destroy() {
	if (instance_) {
		delete instance_;
		instance_ = nullptr;
	}
}

bool CAgoraManager::Init(const char* lpAppID) {
	if (initialized_) {
		printf("[I]: rtc engine has initialized\n");
		return true;
	}

	rtc_engine_ = createAgoraRtcEngine();
	if (!rtc_engine_) {
		printf("[E]: create agora rtc engine fail!\n");
		return false;
	}

	camera_event_handler_ = new CAGEngineEventHandler(this);
	camera_event_handler_->SetConnectionId(DEFAULT_CONNECTION_ID);
	camera_connId_ = DEFAULT_CONNECTION_ID;

	RtcEngineContext ctx;

	ctx.eventHandler = camera_event_handler_;
	ctx.appId = lpAppID;

	int ret = rtc_engine_->initialize(ctx);
	printf("[I]: rtc engine initialize, ret: %d\n", ret);

	if (ret) {
		delete camera_event_handler_;
		camera_event_handler_ = nullptr;

		rtc_engine_->release();
		return false;
	}

	rtc_engine_->queryInterface(agora::rtc::AGORA_IID_MEDIA_ENGINE,
		reinterpret_cast<void**>(&media_engine_));

	if(is_enable_video_observer_) {
		if (!video_frame_observer_) {
			video_frame_observer_ = new VideoFrameObserver();
		}

		if (media_engine_) {
			media_engine_->registerVideoFrameObserver(video_frame_observer_);
		}
	}

	initialized_ = true;

	return true;
}

void CAgoraManager::Release() {
	if (rtc_engine_) {
		rtc_engine_->release();
		rtc_engine_ = nullptr;
	}

	if (media_engine_) {
		media_engine_->release();
		media_engine_ = nullptr;
	}

	if (camera_event_handler_) {
		delete camera_event_handler_;
		camera_event_handler_ = nullptr;
	}

	if (screen_event_handler_) {
		delete screen_event_handler_;
		screen_event_handler_ = nullptr;
	}

	if (custom_event_handler_) {
		delete custom_event_handler_;
		custom_event_handler_ = nullptr;
	}

	if (is_enable_video_observer_ && video_frame_observer_) {
		video_frame_observer_->uninit();
		delete video_frame_observer_;
		video_frame_observer_ = nullptr;
	}

	ResetStates();

	initialized_ = false;
}

bool CAgoraManager::IsJoinChannel() {
	return is_joined_;
}

bool CAgoraManager::JoinChannel(const char* lpChannelId,
		const char* lpToken, agora::rtc::uid_t uid,
		const char* lpSubToken, agora::rtc::uid_t uidSub,
		const char* lpSrcToken, agora::rtc::uid_t uidSrc) {
	RETURN_FALSE_IF_ENGINE_NOT_INITIALIZED()

	if(is_enable_video_observer_ && video_frame_observer_) {
		video_frame_observer_->init();
	}

	ChannelMediaOptions op;
	op.publishAudioTrack = false;
	op.publishCameraTrack = false;
	op.publishScreenTrack = false;
	op.autoSubscribeAudio = false;
	op.autoSubscribeVideo = false;
	op.clientRoleType = CLIENT_ROLE_TYPE::CLIENT_ROLE_BROADCASTER;
	int ret = rtc_engine_->joinChannel(lpToken, lpChannelId, uid, op);
	printf("[I]: joinChannel, ret: %d\n", ret);

	if (!screen_event_handler_) {
		screen_event_handler_ = new CAGEngineEventHandler(this);	
	}

	int ret1 = 0;
	if (uidSub) {
		screen_uid_ = uidSub;

		ChannelMediaOptions op;
		op.publishAudioTrack = false;
		op.publishCameraTrack = false;
		op.publishScreenTrack = false;
		op.autoSubscribeAudio = false;
		op.autoSubscribeVideo = false;
		op.clientRoleType = CLIENT_ROLE_TYPE::CLIENT_ROLE_BROADCASTER;

		ret1 = rtc_engine_->joinChannelEx(lpSubToken, lpChannelId, uidSub, op, screen_event_handler_, &screen_connId_);
		printf("[I]: joinChannelEx, ret1: %d\n", ret1);

		screen_event_handler_->SetConnectionId(screen_connId_);	
	}

	if (!custom_event_handler_) {
		custom_event_handler_ = new CAGEngineEventHandler(this);
	}

	int ret2 = 0;
	if (uidSrc) {
		custom_uid_ = uidSrc;

		ChannelMediaOptions op;
		op.publishAudioTrack = false;
		op.publishCameraTrack = false;
		op.publishScreenTrack = false;
		op.autoSubscribeAudio = false;
		op.autoSubscribeVideo = false;
		op.clientRoleType = CLIENT_ROLE_TYPE::CLIENT_ROLE_BROADCASTER;

		ret2 = rtc_engine_->joinChannelEx(lpSrcToken, lpChannelId, uidSrc, op, custom_event_handler_, &custom_connId_);
		printf("[I]: joinChannelEx, ret2: %d\n", ret2);

		custom_event_handler_->SetConnectionId(custom_connId_);
	}

	return (ret || ret1 || ret2) ? false : true;
}

bool CAgoraManager::LeaveChannel() {
	RETURN_FALSE_IF_ENGINE_NOT_INITIALIZED()

	int ret = rtc_engine_->leaveChannel();
	printf("[I]: leaveChannel, ret: %d\n", ret);

	if (is_enable_video_observer_ && video_frame_observer_) {
		video_frame_observer_->uninit();
	}

	ResetStates();

	return ret ? false : true;
}

void CAgoraManager::SetCameraShowHwnd(HWND hwnd, agora::media::base::RENDER_MODE_TYPE renderMode) {
	RETURN_IF_ENGINE_NOT_INITIALIZED()

	VideoCanvas vc;
	vc.view = hwnd;
	vc.renderMode = renderMode;
	vc.sourceType = VIDEO_SOURCE_CAMERA;
	int ret = rtc_engine_->setupLocalVideo(vc);
	if (ret == 0) {
		camera_view_ = hwnd;
	}
	printf("[I]: setupLocalVideo(camera), ret: %d\n", ret);
}

void CAgoraManager::SetPlayerShowHwnd(agora::rtc::uid_t uid, HWND hwnd, agora::media::base::RENDER_MODE_TYPE renderMode) {
	RETURN_IF_ENGINE_NOT_INITIALIZED()

	VideoCanvas vc;
	vc.view = hwnd;
	vc.uid = uid;
	vc.renderMode = renderMode;
	int ret = rtc_engine_->setupRemoteVideo(vc);
	printf("[I]: setupLocalVideo(screen), ret: %d\n", ret);
}

void CAgoraManager::SetWindowDesktopShowHwnd(HWND hwnd, agora::media::base::RENDER_MODE_TYPE renderMode) {
	RETURN_IF_ENGINE_NOT_INITIALIZED()

	VideoCanvas vc;
	vc.view = hwnd;
	vc.renderMode = renderMode;
	vc.sourceType = VIDEO_SOURCE_SCREEN;
	int ret = rtc_engine_->setupLocalVideo(vc);
	if (ret == 0) {
		screen_view_ = hwnd;
	}
	printf("[I]: setupLocalVideo(screen), ret: %d\n", ret);
}

void CAgoraManager::UpdatePushCameraConfig(int nPushW, int nPushH, int nPushFrameRate) {
	RETURN_IF_ENGINE_NOT_INITIALIZED()

	VideoEncoderConfiguration cfg;
	cfg.dimensions.width = nPushW;
	cfg.dimensions.height = nPushH;
	cfg.frameRate = nPushFrameRate;

	int ret = rtc_engine_->setVideoEncoderConfiguration(cfg);
	printf("[I]: setVideoEncoderConfiguration(publish camera), ret: %d\n", ret);
}

bool CAgoraManager::StartPushCamera(bool bWithMic) {
	RETURN_FALSE_IF_ENGINE_NOT_INITIALIZED()

	if (IsPushCamera()) {
		printf("[I]: StartPushCamera, already push camera\n");
		return true;
	}

	rtc_engine_->enableLocalVideo(true);

	if (bWithMic) {
		rtc_engine_->enableLocalAudio(true);
	}

	ChannelMediaOptions op;
	op.publishAudioTrack = bWithMic;
	op.publishCameraTrack = true;
	int ret1 = rtc_engine_->updateChannelMediaOptions(op, camera_connId_);
	printf("[I]: updateChannelMediaOptions(publish camera), ret: %d\n", ret1);

	if (ret1 == 0) {
		is_publish_camera_ = true;
	}

	rtc_engine_->startPreview();

	return ret1 ? false : true;
}

bool CAgoraManager::StopPushCamera() {
	RETURN_FALSE_IF_ENGINE_NOT_INITIALIZED()

	if (!IsPushCamera()) {
		printf("[I]: StopPushCamera, already stop camera\n");
		return true;
	}

	int ret = rtc_engine_->enableLocalVideo(false);
	int ret1 = rtc_engine_->enableLocalAudio(false);

	printf("[I]: StopPushCamera, ret: %d, ret1: %d\n", ret, ret1);

	if (ret == 0 && ret1 == 0) {
		is_publish_camera_ = false;
	}

	return ((ret == 0 && ret1 == 0) ? false : true);
}

bool CAgoraManager::IsPushCamera() {
	return is_publish_camera_;
}

void CAgoraManager::UpdatePushScreenConfig(int nPushW, int nPushH, int nPushFrameRate) {
	RETURN_IF_ENGINE_NOT_INITIALIZED()

	param_.dimensions.width = nPushW;
	param_.dimensions.height = nPushH;
	param_.frameRate = nPushFrameRate;
	int ret = rtc_engine_->updateScreenCaptureParameters(param_);
	printf("[I]: updateScreenCaptureParameters, ret: %d\n", ret);
}

bool CAgoraManager::StartPushScreen(bool bWithMic, int nPushFps) {
	RETURN_FALSE_IF_ENGINE_NOT_INITIALIZED()

	if (IsPushScreen()) {
		printf("[I]: StartPushScreen, already start screen share\n");
		return true;
	}

	param_.excludeWindowList = reinterpret_cast<agora::view_t *>(exclude_window_list_.data());
	param_.excludeWindowCount = exclude_window_list_.size();
	param_.frameRate = nPushFps;

	int ret = -1;
	int ret1 = -1;

	if (share_win_) {
		ret = rtc_engine_->startScreenCaptureByWindowId((agora::view_t)share_win_, region_rect_, param_);
		printf("[I]: startScreenCaptureByWindowId, ret: %d\n", ret);
	} else {
		ret = rtc_engine_->startScreenCaptureByScreenRect(screen_rect_, region_rect_, param_);
		printf("[I]: startScreenCaptureByScreenRect, ret: %d\n", ret);
	}

	if (ret == 0) {
		ChannelMediaOptions op;
		op.publishScreenTrack = true;
		op.publishAudioTrack = bWithMic;

		if (bWithMic) {
			rtc_engine_->enableLocalAudio(true);
		}

		ret1 = rtc_engine_->updateChannelMediaOptions(op, screen_connId_);
		printf("[I]: updateChannelMediaOptions, ret: %d\n", ret1);
		is_publish_screen_ = true;	
	}

	rtc_engine_->startPreview();

	return ret ? false : true;
}

void CAgoraManager::SetPushDesktop(int nScreenID,
		int x, int y, int w, int h) {
	RETURN_IF_ENGINE_NOT_INITIALIZED()

	std::vector<DesktopInfo> desktops;
	GetDesktopList(desktops);
	if (nScreenID < 0 || nScreenID >= desktops.size()) {
		printf("[E]: SetPushDesktop, desktop_size: %d, screen_id: %d\n", desktops.size(), nScreenID);
		return;
	}

	auto desktop = desktops[nScreenID];
	screen_rect_.x = desktop.x;
	screen_rect_.y = desktop.y;
	screen_rect_.width = desktop.width;
	screen_rect_.height = desktop.height;

	region_rect_.x = x;
	region_rect_.y = y;
	region_rect_.width = w;
	region_rect_.height = h;

	if (IsPushScreen()) {
		rtc_engine_->updateScreenCaptureRegion(region_rect_);
	}
}

bool CAgoraManager::StopPushScreen() {
	RETURN_FALSE_IF_ENGINE_NOT_INITIALIZED()

	if (!IsPushScreen()) {
		printf("[I]: StopPushScreen, already stop screen share\n");
		return true;
	}

	int ret = rtc_engine_->stopScreenCapture();
	printf("[I]: stopScreenCapture, ret: %d\n", ret);
	is_publish_screen_ = false;

	return ret ? false : true;
}

void CAgoraManager::SetPushWindow(HWND hwnd,
		int x, int y, int w, int h) {
	share_win_ = hwnd;
	region_rect_.x = x;
	region_rect_.y = y;
	region_rect_.width = w;
	region_rect_.height = h;
}

void CAgoraManager::GetWindowList(std::vector<WindowInfo>& vWindows) {
	std::list<std::string> filters;
    auto win_list = app::utils::WindowEnumer::EnumAllWindows(filters);
	if (win_list.size()) {
		vWindows.clear();
	}

    for (auto it = win_list.begin(); it != win_list.end(); it++) {
        for (auto item = it->second.begin(); item != it->second.end(); item++) {
			WindowInfo prog;
			prog.sourceId = item->sourceId;
			prog.sourceName = item->sourceName;
			prog.isMinimizeWindow = item->isMinimizeWindow;
			
			prog.x = item->x;
			prog.y = item->y;
			prog.width = item->width;
			prog.height = item->height;
			
			prog.iconBGRA.buffer = (const char*)&item->icon.data[0];
			prog.iconBGRA.length = item->icon.data.size();
			prog.iconBGRA.width = item->icon.width;
			prog.iconBGRA.height = item->icon.height;

			prog.thumbBGRA.buffer = (const char*)&item->thumb.data[0];
			prog.thumbBGRA.length = item->thumb.data.size();
			prog.thumbBGRA.width = item->thumb.width;
			prog.thumbBGRA.height = item->thumb.height;
			vWindows.push_back(prog);
        }
    }
}

void CAgoraManager::GetDesktopList(std::vector<DesktopInfo>& vDesktop) {
	std::list<app::utils::WindowEnumer::MONITOR_INFO> desktops = app::utils::WindowEnumer::EnumAllMonitors();
	if (desktops.size()) {
		vDesktop.clear();
	}

	for (auto it = desktops.begin(); it != desktops.end(); it++) {
		DesktopInfo prog;
		prog.sourceId = it->index;
		prog.isMainScreen = it->is_primary;
		prog.x = it->rc.left;
		prog.y = it->rc.top;
		prog.width = it->rc.right - it->rc.left;
		prog.height = it->rc.bottom - it->rc.top;
		prog.thumbBGRA.buffer = (const char*)&it->thumb.data[0];
		prog.thumbBGRA.length = it->thumb.data.size();
		prog.thumbBGRA.width = it->thumb.width;
		prog.thumbBGRA.height = it->thumb.height;
		vDesktop.push_back(prog);
	}
}

void CAgoraManager::SetPushFilter(HWND* pFilterHwndList, int nFilterNum) {
	if (!pFilterHwndList) {
		return;
	}

	exclude_window_list_.clear();
	for (int i = 0; i < nFilterNum; i++) {
		exclude_window_list_.push_back(pFilterHwndList[i]);
	}

	param_.excludeWindowList = reinterpret_cast<agora::view_t *>(exclude_window_list_.data());
	param_.excludeWindowCount = exclude_window_list_.size();

	int ret = rtc_engine_->updateScreenCaptureParameters(param_);
	printf("[I]: updateScreenCaptureParameters, ret: %d\n", ret);
}

bool CAgoraManager::IsPushScreen() {
	return is_publish_screen_;
}

int CAgoraManager::StartPreview() {
	RETURN_ERR_IF_ENGINE_NOT_INITIALIZED()

	return rtc_engine_->startPreview();
}

int CAgoraManager::StopPreview() {
	RETURN_ERR_IF_ENGINE_NOT_INITIALIZED()

	return rtc_engine_->stopPreview();
}

void CAgoraManager::EnableVideoFrameObserver(bool enable) {
	is_enable_video_observer_ = enable;
}

bool CAgoraManager::GetPlayerImageSize(agora::rtc::uid_t uid, int& nRetW, int& nRetH) {
	if (video_frame_observer_) {
		return video_frame_observer_->GetPlayerImageSize(uid, nRetW, nRetH);
	}

	return false;
}

bool CAgoraManager::GetPlayerImage(agora::rtc::uid_t uid, BYTE* pData, int& nRetW, int& nRetH) {
	if (video_frame_observer_) {
		return video_frame_observer_->getPlayerImage(uid, pData, nRetW, nRetH);
	}

	return false;
}

bool CAgoraManager::GetCameraImageSize(int& nRetW, int& nRetH) {
	if (video_frame_observer_) {
		return video_frame_observer_->GetCameraImageSize(nRetW, nRetH);
	}

	return false;
}

bool CAgoraManager::GetCameraImage(BYTE* pData, int& nRetW, int& nRetH) {
	if (video_frame_observer_) {
		return video_frame_observer_->getCameraImage(pData, nRetW, nRetH);
	}

	return false;
}

bool CAgoraManager::GetScreenImageSize(int& nRetW, int& nRetH) {
	if (video_frame_observer_) {
		return video_frame_observer_->GetScreenImageSize(nRetW, nRetH);
	}

	return false;
}

bool CAgoraManager::GetScreenImage(BYTE* pData, int& nRetW, int& nRetH) {
	if (video_frame_observer_) {
		return video_frame_observer_->getScreenImage(pData, nRetW, nRetH);
	}
}

void CAgoraManager::SetPushCameraPause(bool bPause) {
	RETURN_IF_ENGINE_NOT_INITIALIZED()

	ChannelMediaOptions op;
	op.publishCameraTrack = (!bPause);
	int ret = rtc_engine_->updateChannelMediaOptions(op, camera_connId_);
	printf("[I]: updateChannelMediaOptions(camera), mute: %d, ret: %d\n", bPause, ret);
	if (ret == 0) {
		is_mute_camera_ = bPause;
	}
}

bool CAgoraManager::IsPushCameraPause() {
	return is_mute_camera_;
}

void CAgoraManager::SetPushCameraAudioMute(bool bMute) {
	RETURN_IF_ENGINE_NOT_INITIALIZED()

	int ret = rtc_engine_->muteLocalAudioStream(bMute);
	printf("[I]: muteLocalAudioStream, mute: %d, ret: %d\n", bMute, ret);
	if (ret == 0) {
		is_mute_mic_ = bMute;
	}
}

bool CAgoraManager::IsPushCameraAudioMute() {
	return is_mute_mic_;
}

void CAgoraManager::SetPushCamera(int nCamID) {
	RETURN_IF_ENGINE_NOT_INITIALIZED()

	agora::rtc::IVideoDeviceManager* vdm = nullptr;
    rtc_engine_->queryInterface(agora::rtc::AGORA_IID_VIDEO_DEVICE_MANAGER,
                          reinterpret_cast<void**>(&vdm));
	if (!vdm) {
		return;
	}

	std::unique_ptr<agora::rtc::IVideoDeviceManager> video_device_manager(vdm);

	std::vector<CameraInfo> camera_list;
	GetCameraList(camera_list);
	for (int i = 0; i < camera_list.size(); i++) {
		if (nCamID == camera_list[i].idx) {
			video_device_manager->setDevice(camera_list[i].device_id.c_str());
			break;
		}
	}
}

void CAgoraManager::GetCameraList(std::vector<CameraInfo>& vCamera) {
	RETURN_IF_ENGINE_NOT_INITIALIZED()

	agora::rtc::IVideoDeviceManager* vdm = nullptr;
    rtc_engine_->queryInterface(agora::rtc::AGORA_IID_VIDEO_DEVICE_MANAGER,
                          reinterpret_cast<void**>(&vdm));
	if (!vdm) {
		return;
	}

	std::unique_ptr<agora::rtc::IVideoDeviceManager> video_device_manager(vdm);
    std::unique_ptr<agora::rtc::IVideoDeviceCollection> vdc(video_device_manager->enumerateVideoDevices());

	int count = vdc->getCount();
	if (count <= 0) {
		return;
	}

	char device_name_utf8[agora::rtc::MAX_DEVICE_ID_LENGTH] = {0};
    char device_id[agora::rtc::MAX_DEVICE_ID_LENGTH] = {0};
	CameraInfo camera_prog;

	for (int i = 0; i < count; i++) {
		memset(device_name_utf8, 0, agora::rtc::MAX_DEVICE_ID_LENGTH);
		memset(device_id, 0, agora::rtc::MAX_DEVICE_ID_LENGTH);
		vdc->getDevice(i, device_name_utf8, device_id);
		camera_prog.idx = i;
		camera_prog.device_name_utf8 = std::string(device_name_utf8);
		camera_prog.device_id = std::string(device_id);
		vCamera.push_back(camera_prog);
	}
}

void CAgoraManager::SetMic(int nID) {
	RETURN_IF_ENGINE_NOT_INITIALIZED()

	agora::rtc::IAudioDeviceManager* adm = nullptr;
    rtc_engine_->queryInterface(agora::rtc::AGORA_IID_AUDIO_DEVICE_MANAGER,
                          reinterpret_cast<void**>(&adm));
	if (!adm) {
		return;
	}

	std::unique_ptr<agora::rtc::IAudioDeviceManager> audio_device_manager(adm);

	std::vector<MicInfo> mic_list;
	GetMicList(mic_list);
	for (int i = 0; i < mic_list.size(); i++) {
		if (nID == mic_list[i].idx) {
			audio_device_manager->setRecordingDevice(mic_list[i].device_id.c_str());
			break;
		}
	}
}

void CAgoraManager::GetMicList(std::vector<MicInfo>& vMic) {
	RETURN_IF_ENGINE_NOT_INITIALIZED()

	agora::rtc::IAudioDeviceManager* adm = nullptr;
    rtc_engine_->queryInterface(agora::rtc::AGORA_IID_AUDIO_DEVICE_MANAGER,
                          reinterpret_cast<void**>(&adm));
	if (!adm) {
		return;
	}

	std::unique_ptr<agora::rtc::IAudioDeviceManager> audio_device_manager(adm);
    std::unique_ptr<agora::rtc::IAudioDeviceCollection> adc(audio_device_manager->enumerateRecordingDevices());

	int count = adc->getCount();
	if (count <= 0) {
		return;
	}

	char device_name_utf8[agora::rtc::MAX_DEVICE_ID_LENGTH] = {0};
    char device_id[agora::rtc::MAX_DEVICE_ID_LENGTH] = {0};
	MicInfo mic_prog;

	for (int i = 0; i < count; i++) {
		memset(device_name_utf8, 0, agora::rtc::MAX_DEVICE_ID_LENGTH);
		memset(device_id, 0, agora::rtc::MAX_DEVICE_ID_LENGTH);
		adc->getDevice(i, device_name_utf8, device_id);
		mic_prog.idx = i;
		mic_prog.device_name_utf8 = std::string(device_name_utf8);
		mic_prog.device_id = std::string(device_id);
		vMic.push_back(mic_prog);
	}
}

void CAgoraManager::SetMicVolume(int nVol) {
	RETURN_IF_ENGINE_NOT_INITIALIZED()
	int ret = rtc_engine_->adjustRecordingSignalVolume(nVol);
	printf("[I]: adjustRecordingSignalVolume, ret: %d\n", ret);
}

void CAgoraManager::SetSystemMicVolume(int nVol) {
	RETURN_IF_ENGINE_NOT_INITIALIZED()

	agora::rtc::IAudioDeviceManager* adm = nullptr;
	rtc_engine_->queryInterface(agora::rtc::AGORA_IID_AUDIO_DEVICE_MANAGER,
	  reinterpret_cast<void**>(&adm));
	if (!adm) {
	  return;
	}

	std::unique_ptr<agora::rtc::IAudioDeviceManager> audio_device_manager(adm);
	int ret = audio_device_manager->setRecordingDeviceVolume(nVol);
	printf("[I]: setRecordingDeviceVolume, ret: %d\n", ret);
}

void CAgoraManager::SetPushSystemAudio(int nMode) {
	if (nMode < PushSystemAudioOption_None || nMode > PushSystemAudioOption_Screen) {
		printf("[E]: invalid system audio mode, mode: %d\n", nMode);
		return;
	}

	if (nMode == current_recording_mode_) {
		return;
	}

	if (current_recording_mode_ == PushSystemAudioOption_Camera) {
		rtc_engine_->enableLoopbackRecording(camera_connId_, false);
	}
	else if (current_recording_mode_ == PushSystemAudioOption_Screen) {
		rtc_engine_->enableLoopbackRecording(screen_connId_, false);
	}

	if (PushSystemAudioOption_None != nMode) {
		conn_id_t conn_id = agora::rtc::DUMMY_CONNECTION_ID;
		if (nMode == PushSystemAudioOption_Camera) {
			conn_id = camera_connId_;
		}else if (nMode == PushSystemAudioOption_Screen) {
			conn_id = screen_connId_;
		}
		rtc_engine_->enableLoopbackRecording(conn_id, true);
	}

	current_recording_mode_ = nMode;
}

void CAgoraManager::StartPlayer(agora::rtc::uid_t uid) {
	RETURN_IF_ENGINE_NOT_INITIALIZED()

	int ret = rtc_engine_->muteRemoteVideoStream(uid, false);
	int ret1 = rtc_engine_->muteRemoteAudioStream(uid, false);
	printf("[I]: muteRemoteVideoStream(false), ret: %d\n", ret);
	printf("[I]: muteRemoteAudioStream(false), ret: %d\n", ret1);
}

void CAgoraManager::StopPlayer(agora::rtc::uid_t uid) {
	RETURN_IF_ENGINE_NOT_INITIALIZED()

	int ret = rtc_engine_->muteRemoteVideoStream(uid, true);
	int ret1 = rtc_engine_->muteRemoteAudioStream(uid, true);
	printf("[I]: muteRemoteVideoStream(true), ret: %d\n", ret);
	printf("[I]: muteRemoteAudioStream(true), ret: %d\n", ret1);
}

void CAgoraManager::PausePlayer(agora::rtc::uid_t uid, bool bPause) {
	RETURN_IF_ENGINE_NOT_INITIALIZED()

	int ret = rtc_engine_->muteRemoteVideoStream(uid, bPause);
	printf("[I]: muteRemoteVideoStream(camera), mute: %d, ret: %d\n", bPause, ret);
}

void CAgoraManager::MutePlayer(agora::rtc::uid_t uid, bool bMute) {
	RETURN_IF_ENGINE_NOT_INITIALIZED()

	int ret = rtc_engine_->muteRemoteAudioStream(uid, bMute);
	printf("[I]: muteRemoteAudioStream(camera), mute: %d, ret: %d\n", bMute, ret);
}

void CAgoraManager::GetPlayerUID(std::vector<agora::rtc::uid_t>& uidList) {
	RETURN_IF_ENGINE_NOT_INITIALIZED()

	for (auto it = users_in_channel_.begin(); it != users_in_channel_.end(); it++) {
		uidList.push_back(*it);
	}
}

void CAgoraManager::OnJoinChannelSuccess(const char* channel, uid_t uid, int elapsed, conn_id_t connId) {
	if (connId == DEFAULT_CONNECTION_ID) {
		camera_uid_ = uid;
		is_joined_ = true;
	} else {
		
	}
}

void CAgoraManager::SetPushScreenPause(bool bPause) {
	RETURN_IF_ENGINE_NOT_INITIALIZED()

	ChannelMediaOptions op;
	op.publishScreenTrack = (!bPause);
	int ret = rtc_engine_->updateChannelMediaOptions(op, screen_connId_);
	printf("[I]: updateChannelMediaOptions(camera), mute: %d, ret: %d\n", bPause, ret);
	if (ret == 0) {
		is_mute_screen_ = bPause;
	}
}

bool CAgoraManager::IsPushScreenPause() {
	return is_mute_screen_;
}

void CAgoraManager::SetPushScreenAudioMute(bool bMute) {
	RETURN_IF_ENGINE_NOT_INITIALIZED()

	int ret = rtc_engine_->muteLocalAudioStream(bMute);
	printf("[I]: muteLocalAudioStream, mute: %d, ret: %d\n", bMute, ret);
	if (ret == 0) {
		is_mute_mic_ = bMute;
	}
}

bool CAgoraManager::IsPushScreenAudioMute() {
	return is_mute_mic_;
}

void CAgoraManager::OnLeaveChannel(const RtcStats& stat, conn_id_t connId) {
}

void CAgoraManager::OnUserJoined(uid_t uid, int elapsed, conn_id_t connId) {
	if (connId == DEFAULT_CONNECTION_ID) {
		if (uid != screen_uid_ && uid != custom_uid_) {
			users_in_channel_.insert(uid);
		} else {
			rtc_engine_->muteRemoteAudioStream(uid, true);
			rtc_engine_->muteRemoteVideoStream(uid, true);
		}
	}
}

void CAgoraManager::OnUserOffline(uid_t uid, USER_OFFLINE_REASON_TYPE reason, conn_id_t connId) {
	if (connId == DEFAULT_CONNECTION_ID) {
		users_in_channel_.erase(uid);
	}
}

void CAgoraManager::onError(int err, const char* msg, conn_id_t connId) {
	if (connId == DEFAULT_CONNECTION_ID) {
		printf("[I]: onError, err: %d, msg: %s\n", err, msg);
	}
}

void CAgoraManager::onConnectionStateChanged(CONNECTION_STATE_TYPE state, CONNECTION_CHANGED_REASON_TYPE reason, conn_id_t connId) {
	printf("[I]: onConnectionStateChanged, state: %d, reason: %d, connId: %d\n", state, reason, connId);
}

void CAgoraManager::ResetStates() {
	is_joined_ = false;
	is_publish_camera_ = false;
	is_publish_screen_ = false;
	is_publish_custom_ = false;
	is_mute_camera_ = false;
	is_mute_screen_ = false;
	is_mute_mic_ = false;
	is_preview_ = false;
	is_enable_video_observer_ = false;

	push_screen_width_ = 0;
	push_screen_height_ = 0;

	share_win_ = 0;

	current_recording_mode_ = 0;

	camera_uid_ = 0;
	screen_uid_ = 0;
	custom_uid_ = 0;

	param_ = ScreenCaptureParameters();
	screen_rect_ = agora::rtc::Rectangle();
	region_rect_ = agora::rtc::Rectangle();
	exclude_window_list_.clear();
	users_in_channel_.clear();
}

bool CAgoraManager::StartPushCustom()
{
	RETURN_FALSE_IF_ENGINE_NOT_INITIALIZED()

	ChannelMediaOptions op;
	op.publishCustomAudioTrack = true;
	op.publishCustomVideoTrack = true;
	int ret = rtc_engine_->updateChannelMediaOptions(op, custom_connId_);
	printf("[I]: updateChannelMediaOptions(publish custom), ret: %d\n", ret);

	if (ret == 0) {
		is_publish_custom_ = true;
	}

	ret = rtc_engine_->startPreview();
	return ret == 0 ? true : false;
}

bool CAgoraManager::StopPushCustom()
{
	RETURN_FALSE_IF_ENGINE_NOT_INITIALIZED()

	ChannelMediaOptions op;
	op.publishCustomAudioTrack = false;
	op.publishCustomVideoTrack = false;
	int ret = rtc_engine_->updateChannelMediaOptions(op, custom_connId_);
	printf("[I]: updateChannelMediaOptions(unpublish custom), ret: %d\n", ret);

	if (ret == 0) {
		is_publish_custom_ = false;
	}

	return ret ? false : true;
}

bool CAgoraManager::IsPushCustom()
{
	return is_publish_custom_;
}

bool CAgoraManager::PushVideoFrame(unsigned char * pData, int nW, int nH, long long ms)
{
	RETURN_FALSE_IF_ENGINE_NOT_INITIALIZED()

	agora::media::base::ExternalVideoFrame frame;
	frame.buffer = pData;
	frame.format = agora::media::base::VIDEO_PIXEL_RGBA;
	frame.height = nH;
	frame.stride = nW;
	frame.timestamp = ms;
	int ret = media_engine_->pushVideoFrame(&frame, custom_connId_);
	return ret == 0 ? true : false;
}

bool CAgoraManager::PushAudioFrame(unsigned char * pData, int nSize, long lSampleRate, int nChannel, long long ms)
{
	RETURN_FALSE_IF_ENGINE_NOT_INITIALIZED()

	agora::media::IAudioFrameObserver::AudioFrame frame;
	frame.type = agora::media::IAudioFrameObserver::FRAME_TYPE_PCM16;
	frame.avsync_type = 0;
	frame.buffer = pData;
	frame.channels = nChannel;
	frame.bytesPerSample = BYTES_PER_SAMPLE::TWO_BYTES_PER_SAMPLE;
	frame.samplesPerSec = lSampleRate;
	frame.samplesPerChannel = lSampleRate / 100;
	frame.renderTimeMs = ms;
	
	int ret = media_engine_->pushAudioFrame(agora::media::AUDIO_RECORDING_SOURCE, &frame, false, 0, custom_connId_);
	return ret == 0 ? true : false;
}

