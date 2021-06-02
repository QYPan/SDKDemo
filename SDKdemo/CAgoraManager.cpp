#include "stdafx.h"
#include "CAgoraManager.h"
#include "VideoFrameObserver.h"
#include "CAGEngineEventHandler.h"

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

CAgoraManager* CAgoraManager::Inst() {
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

	if(is_enable_video_observer_) {
		if (!video_frame_observer_) {
			video_frame_observer_ = new VideoFrameObserver();
		}

		rtc_engine_->queryInterface(agora::rtc::AGORA_IID_MEDIA_ENGINE,
                          reinterpret_cast<void**>(&media_engine_));

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

	if (is_enable_video_observer_ && video_frame_observer_) {
		video_frame_observer_->uninit();
		delete video_frame_observer_;
		video_frame_observer_ = nullptr;
	}

	RestStates();

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
		printf("[I]: joinChannelEx, ret: %d\n", ret1);

		screen_event_handler_->SetConnectionId(screen_connId_);	
	}

	return (ret || ret1) ? false : true;
}

bool CAgoraManager::LeaveChannel() {
	RETURN_FALSE_IF_ENGINE_NOT_INITIALIZED()

	int ret = rtc_engine_->leaveChannel();
	printf("[I]: leaveChannel, ret: %d\n", ret);

	if (is_enable_video_observer_ && video_frame_observer_) {
		video_frame_observer_->uninit();
	}

	RestStates();

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

	ChannelMediaOptions op;
	op.publishCameraTrack = false;
	int ret = rtc_engine_->updateChannelMediaOptions(op, camera_connId_);
	printf("[I]: updateChannelMediaOptions(unpublish camera), ret: %d\n", ret);

	if (ret == 0) {
		is_publish_camera_ = false;
	}

	return ret ? false : true;
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

bool CAgoraManager::StartPushScreen(bool bWithMic) {
	RETURN_FALSE_IF_ENGINE_NOT_INITIALIZED()

	param_.excludeWindowList = reinterpret_cast<agora::view_t *>(exclude_window_list_.data());
	param_.excludeWindowCount = exclude_window_list_.size();
	param_.frameRate = 15;

	int ret = -1;
	int ret1 = -1;

	if (share_win_) {
		ret = rtc_engine_->startScreenCaptureByWindowId((agora::view_t)share_win_, region_rect_, param_);
		printf("[I]: startScreenCaptureByWindowId, ret: %d\n", ret);
	} else {
		ret = rtc_engine_->startScreenCaptureByScreenRect(agora::rtc::Rectangle(), region_rect_, param_);
		printf("[I]: startScreenCaptureByScreenRect, ret: %d\n", ret);
	}

	if (ret == 0) {
		ChannelMediaOptions op;
		op.publishScreenTrack = true;

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

int CAgoraManager::GetPlayerImageW(agora::rtc::uid_t uid) {
	if (video_frame_observer_) {
		return video_frame_observer_->getPlayerImageW(uid);
	}

	return 0;
}

int CAgoraManager::GetPlayerImageH(agora::rtc::uid_t uid) {
	if (video_frame_observer_) {
		return video_frame_observer_->getPlayerImageH(uid);
	}

	return 0;
}

bool CAgoraManager::GetPlayerImage(agora::rtc::uid_t uid, BYTE* pData, int& nRetW, int& nRetH) {
	if (video_frame_observer_) {
		return video_frame_observer_->getPlayerImage(uid, pData, nRetW, nRetH);
	}

	return false;
}

int CAgoraManager::GetCameraImageW() {
	if (video_frame_observer_) {
		return video_frame_observer_->getCameraImageW();
	}

	return 0;
}

int CAgoraManager::GetCameraImageH() {
	if (video_frame_observer_) {
		return video_frame_observer_->getCameraImageH();
	}
}

bool CAgoraManager::GetCameraImage(BYTE* pData, int& nRetW, int& nRetH) {
	if (video_frame_observer_) {
		return video_frame_observer_->getCameraImage(pData, nRetW, nRetH);
	}

	return false;
}

int CAgoraManager::GetScreenImageW() {
	if (video_frame_observer_) {
		return video_frame_observer_->getScreenImageW();
	}
}

int CAgoraManager::GetScreenImageH() {
	if (video_frame_observer_) {
		return video_frame_observer_->getScreenImageH();
	}
}

bool CAgoraManager::GetScreenImage(BYTE* pData, int& nRetW, int& nRetH) {
	if (video_frame_observer_) {
		return video_frame_observer_->getScreenImage(pData, nRetW, nRetH);
	}
}

void CAgoraManager::SetPushCameraPause(bool bPause) {
	RETURN_IF_ENGINE_NOT_INITIALIZED()

	int ret = rtc_engine_->muteLocalVideoStream(bPause);
	printf("[I]: muteLocalVideoStream, mute: %d, ret: %d\n", bPause, ret);
	if (ret == 0) {
		is_mute_camera_ = bPause;
	}
}

bool CAgoraManager::IsPushCameraPause() {
	return is_mute_camera_;
}

void CAgoraManager::SetPushCameraMicMute(bool bMute) {
	RETURN_IF_ENGINE_NOT_INITIALIZED()

	int ret = rtc_engine_->muteLocalAudioStream(bMute);
	printf("[I]: muteLocalAudioStream, mute: %d, ret: %d\n", bMute, ret);
	if (ret == 0) {
		is_mute_mic_ = bMute;
	}
}

bool CAgoraManager::IsPushCameraMicMute() {
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

	std::vector<CameraProg> camera_list;
	GetCameraList(camera_list);
	for (int i = 0; i < camera_list.size(); i++) {
		if (nCamID == camera_list[i].idx) {
			video_device_manager->setDevice(camera_list[i].device_id.c_str());
			break;
		}
	}
}

void CAgoraManager::GetCameraList(std::vector<CameraProg>& vCamera) {
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
	CameraProg camera_prog;

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

	std::vector<MicProg> mic_list;
	GetMicList(mic_list);
	for (int i = 0; i < mic_list.size(); i++) {
		if (nID == mic_list[i].idx) {
			audio_device_manager->setRecordingDevice(mic_list[i].device_id.c_str());
			break;
		}
	}
}

void CAgoraManager::GetMicList(std::vector<MicProg>& vMic) {
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
	MicProg mic_prog;

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

	// - 0: Mute the recording volume.
    // - 100: The Original volume.
    // - 400: (Maximum) Four times the original volume with signal clipping
	int ret = rtc_engine_->adjustRecordingSignalVolume(nVol);
	printf("[I]: adjustRecordingSignalVolume, ret: %d\n", ret);
}

void CAgoraManager::SetPushSystemAudio(int nMode) {
	if (nMode < 0 || nMode > 2) {
		printf("[E]: invalid system audio mode, mode: %d\n", nMode);
		return;
	}

	if (nMode == current_recording_mode_) {
		return;
	}

	conn_id_t conn_id = agora::rtc::DUMMY_CONNECTION_ID;
	if (nMode == 1) {
		conn_id = camera_connId_;
	} else if (nMode == 2) {
		conn_id = screen_connId_;
	}

	rtc_engine_->enableLoopbackRecording(false, current_recording_mode_);

	if (nMode) {
		rtc_engine_->enableLoopbackRecording(true, nMode);
	}

	current_recording_mode_ = nMode;
}

void CAgoraManager::PlayVideo(agora::rtc::uid_t uid) {
	RETURN_IF_ENGINE_NOT_INITIALIZED()

	int ret = rtc_engine_->muteRemoteVideoStream(uid, false);
	int ret1 = rtc_engine_->muteRemoteAudioStream(uid, false);
	printf("[I]: muteRemoteVideoStream(false), ret: %d\n", ret);
	printf("[I]: muteRemoteAudioStream(false), ret: %d\n", ret1);
}

void CAgoraManager::StopVideo(agora::rtc::uid_t uid) {
	RETURN_IF_ENGINE_NOT_INITIALIZED()

	int ret = rtc_engine_->muteRemoteVideoStream(uid, true);
	int ret1 = rtc_engine_->muteRemoteAudioStream(uid, true);
	printf("[I]: muteRemoteVideoStream(true), ret: %d\n", ret);
	printf("[I]: muteRemoteAudioStream(true), ret: %d\n", ret1);
}

void CAgoraManager::MuteVideo(agora::rtc::uid_t uid, bool bMute) {
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

void CAgoraManager::SetScreenPushPause(bool bPause) {
	if (bPause) {
		ChannelMediaOptions op;
		op.publishScreenTrack = false;
		op.clientRoleType = CLIENT_ROLE_TYPE::CLIENT_ROLE_BROADCASTER;
		rtc_engine_->updateChannelMediaOptions(op, screen_connId_);

		int ret = rtc_engine_->stopScreenCapture();
		printf("[I]: stopScreenCapture, ret: %d\n", ret);
		ScreenCaptureParameters param;
		param.dimensions.width = push_screen_width_;
		param.dimensions.height = push_screen_height_;
		param.excludeWindowList = reinterpret_cast<agora::view_t *>(exclude_window_list_.data());
		param.excludeWindowCount = exclude_window_list_.size();

		if (share_win_) {
			ret = rtc_engine_->startScreenCaptureByWindowId((agora::view_t)share_win_, region_rect_, param);
			printf("[I]: startScreenCaptureByWindowId, ret: %d\n", ret);
		} else {
			ret = rtc_engine_->startScreenCaptureByScreenRect(agora::rtc::Rectangle(), region_rect_, param);
			printf("[I]: startScreenCaptureByScreenRect, ret: %d\n", ret);
		}

	} else {
		ChannelMediaOptions op;
		op.publishScreenTrack = true;
		op.clientRoleType = CLIENT_ROLE_TYPE::CLIENT_ROLE_BROADCASTER;
		rtc_engine_->updateChannelMediaOptions(op, screen_connId_);
	}

	is_mute_screen_ = bPause;
}

bool CAgoraManager::IsScreenPushPause() {
	return is_mute_screen_;
}

void CAgoraManager::SetPushScreenMicMute(bool bMute) {
	RETURN_IF_ENGINE_NOT_INITIALIZED()

	int ret = rtc_engine_->muteLocalAudioStream(bMute);
	printf("[I]: muteLocalAudioStream, mute: %d, ret: %d\n", bMute, ret);
	if (ret == 0) {
		is_mute_mic_ = bMute;
	}
}

bool CAgoraManager::IsPushScreenMicMute() {
	return is_mute_mic_;
}

void CAgoraManager::OnLeaveChannel(const RtcStats& stat, conn_id_t connId) {
}

void CAgoraManager::OnUserJoined(uid_t uid, int elapsed, conn_id_t connId) {
	if (connId == DEFAULT_CONNECTION_ID) {
		if (uid != screen_uid_) {
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

void CAgoraManager::RestStates() {
	is_joined_ = false;
	is_publish_camera_ = false;
	is_publish_screen_ = false;
	is_mute_camera_ = false;
	is_mute_screen_ = false;
	is_mute_mic_ = false;
	is_preview_ = false;
	is_enable_video_observer_ = false;

	push_screen_width_ = 0;
	push_screen_height_ = 0;

	share_win_ = 0;

	camera_uid_ = 0;
	screen_uid_ = 0;

	param_ = ScreenCaptureParameters();
	region_rect_ = agora::rtc::Rectangle();
	exclude_window_list_.clear();
	users_in_channel_.clear();
}

