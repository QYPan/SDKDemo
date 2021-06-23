#include "stdafx.h"
#include "CAgoraManager.h"
#include "VideoFrameObserver.h"
#include "CAGEngineEventHandler.h"
#include "WinEnumerImpl.h"

#include <fstream>

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

class SimpleLogger {
public:
  static SimpleLogger* GetInstance() {
    if (!instance_) {
      instance_ = new SimpleLogger();
    }
    return instance_;
  }

  static void DestroyInstance() {
    if (instance_) {
      delete instance_;
      instance_ = nullptr;
    }
  }

  enum LOG_TYPE {
    L_INFO,
    L_WARN,
    L_ERROR,
	L_FUNC
  };

  void Print(LOG_TYPE type, const char* fmt, ...) {
    if (!fmt || !*fmt) {
      return;
    }

    va_list ap;
    va_start(ap, fmt);
    auto size = vsnprintf(nullptr, 0, fmt, ap);
    va_end(ap);
    if (size <= 0) {
      return;
    }

    std::unique_ptr<char[]> buf = std::make_unique<char[]>(size + 2);
    memset(buf.get(), 0, size + 2);
    va_start(ap, fmt);
    size = vsnprintf(buf.get(), size + 2, fmt, ap);
    va_end(ap);
    if (size <= 0) {
      return;
    }

	static std::map<LOG_TYPE, std::string> log_type_map = {{L_INFO, "[I]: "},{L_WARN, "[W]: "},{L_ERROR, "[E]: "}, {L_FUNC, "[F]: "}};

    std::string msg(buf.get());
    if (writer_.is_open()) {
      writer_ << log_type_map[type] << msg << std::endl;
    }
  }

private:
  SimpleLogger() {
    writer_.open("AgoraManager.log", std::ofstream::out);
  }

  ~SimpleLogger() {
    writer_.close();
  }

  static SimpleLogger *instance_;
  std::ofstream writer_;
};

SimpleLogger* SimpleLogger::instance_ = nullptr;

#define PRINT_LOG(type, ...) \
	SimpleLogger::GetInstance()->Print(type, ##__VA_ARGS__)

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
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_FUNC, __FUNCTION__);
	if (initialized_) {
		PRINT_LOG(SimpleLogger::LOG_TYPE::L_INFO, "rtc engine has initialized.");
		return true;
	}

	rtc_engine_ = createAgoraRtcEngine();
	if (!rtc_engine_) {
		PRINT_LOG(SimpleLogger::LOG_TYPE::L_ERROR, "create agora rtc engine fail!");
		return false;
	}

	camera_event_handler_ = new CAGEngineEventHandler(this);
	camera_event_handler_->SetConnectionId(DEFAULT_CONNECTION_ID);
	camera_connId_ = DEFAULT_CONNECTION_ID;

	RtcEngineContext ctx;

	ctx.eventHandler = camera_event_handler_;
	ctx.appId = lpAppID;

	int ret = rtc_engine_->initialize(ctx);
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_INFO, "rtc engine initialize, ret: %d.", ret);

	if (ret) {
		delete camera_event_handler_;
		camera_event_handler_ = nullptr;

		rtc_engine_->release();
		return false;
	}

	rtc_engine_->queryInterface(agora::rtc::AGORA_IID_MEDIA_ENGINE,
		reinterpret_cast<void**>(&media_engine_));

	if (is_enable_video_observer_) {
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
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_FUNC, __FUNCTION__);
	RETURN_IF_ENGINE_NOT_INITIALIZED()

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

	SimpleLogger::DestroyInstance();
}

bool CAgoraManager::IsJoinChannel() {
	return is_joined_;
}

bool CAgoraManager::JoinChannel(const char* lpChannelId,
	const char* lpToken, agora::rtc::uid_t uid,
	const char* lpSubToken, agora::rtc::uid_t uidSub,
	const char* lpSrcToken, agora::rtc::uid_t uidSrc) {
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_FUNC, "%s: channel: %s, uid: %u, uidSub: %u, uidSrc: %u.", __FUNCTION__, lpChannelId, uid, uidSub, uidSrc);
	RETURN_FALSE_IF_ENGINE_NOT_INITIALIZED()

	if (is_enable_video_observer_ && video_frame_observer_) {
		video_frame_observer_->init();
		PRINT_LOG(SimpleLogger::LOG_TYPE::L_INFO, "enable video frame observer.");
	}

	ChannelMediaOptions op;
	op.publishAudioTrack = false;
	op.publishCameraTrack = false;
	op.publishScreenTrack = false; 
	op.autoSubscribeAudio = false;
	op.autoSubscribeVideo = false;
	op.clientRoleType = CLIENT_ROLE_TYPE::CLIENT_ROLE_AUDIENCE;
	int ret = rtc_engine_->joinChannel(lpToken, lpChannelId, uid, op);
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_INFO, "joinChannel, channel: %s, uid: %u, ret: %d.", lpChannelId, uid, ret);

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
		op.clientRoleType = CLIENT_ROLE_TYPE::CLIENT_ROLE_AUDIENCE;

		ret1 = rtc_engine_->joinChannelEx(lpSubToken, lpChannelId, uidSub, op, screen_event_handler_, &screen_connId_);
		PRINT_LOG(SimpleLogger::LOG_TYPE::L_INFO, "joinChannelEx, channel: %s, uid: %u, ret1: %d.", lpChannelId, uidSub, ret1);

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
		op.clientRoleType = CLIENT_ROLE_TYPE::CLIENT_ROLE_AUDIENCE;

		ret2 = rtc_engine_->joinChannelEx(lpSrcToken, lpChannelId, uidSrc, op, custom_event_handler_, &custom_connId_);
		PRINT_LOG(SimpleLogger::LOG_TYPE::L_INFO, "joinChannelEx, channel: %s, uid: %u, ret2: %d.", lpChannelId, uidSrc, ret2);

		custom_event_handler_->SetConnectionId(custom_connId_);
	}

	return (ret || ret1 || ret2) ? false : true;
}

bool CAgoraManager::LeaveChannel() {
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_FUNC, __FUNCTION__);
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
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_FUNC, "%s: hwnd: %d, renderMode: %d.", __FUNCTION__, hwnd, renderMode);
	RETURN_IF_ENGINE_NOT_INITIALIZED()

	VideoCanvas vc;
	vc.view = hwnd;
	vc.renderMode = renderMode;
	vc.sourceType = VIDEO_SOURCE_CAMERA;
	int ret = rtc_engine_->setupLocalVideo(vc);
	if (ret == 0) {
		camera_view_ = hwnd;
	}
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_INFO, "setupLocalVideo, ret: %d.", ret);
}

void CAgoraManager::SetPlayerShowHwnd(agora::rtc::uid_t uid, HWND hwnd, agora::media::base::RENDER_MODE_TYPE renderMode) {
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_FUNC, "%s: uid: %u, hwnd: %d, renderMode: %d.", __FUNCTION__, uid, hwnd, renderMode);
	RETURN_IF_ENGINE_NOT_INITIALIZED()

	VideoCanvas vc;
	vc.view = hwnd;
	vc.uid = uid;
	vc.renderMode = renderMode;
	int ret = rtc_engine_->setupRemoteVideo(vc);
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_INFO, "setupRemoteVideo, ret: %d.", ret);
}

void CAgoraManager::SetWindowDesktopShowHwnd(HWND hwnd, agora::media::base::RENDER_MODE_TYPE renderMode) {
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_FUNC, "%s: hwnd: %d, renderMode: %d.", __FUNCTION__, hwnd, renderMode);
	RETURN_IF_ENGINE_NOT_INITIALIZED()

	VideoCanvas vc;
	vc.view = hwnd;
	vc.renderMode = renderMode;
	vc.mirrorMode = agora::rtc::VIDEO_MIRROR_MODE_DISABLED;
	vc.sourceType = VIDEO_SOURCE_SCREEN;
	int ret = rtc_engine_->setupLocalVideo(vc);
	if (ret == 0) {
		screen_view_ = hwnd;
	}
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_INFO, "setupLocalVideo, ret: %d.", ret);
}

void CAgoraManager::UpdatePushCameraConfig(int nPushW, int nPushH, int nPushFrameRate) {
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_FUNC, "%s: nPushW: %d, nPushH: %d, nPushFrameRate: %d.", __FUNCTION__, nPushW, nPushH, nPushFrameRate);
	RETURN_IF_ENGINE_NOT_INITIALIZED()

	VideoEncoderConfiguration cfg;
	cfg.dimensions.width = nPushW;
	cfg.dimensions.height = nPushH;
	cfg.frameRate = nPushFrameRate;

	int ret = rtc_engine_->setVideoEncoderConfiguration(cfg);
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_INFO, "setVideoEncoderConfiguration, ret: %d.", ret);
}

bool CAgoraManager::StartPushCamera(bool bWithMic) {
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_FUNC, "%s: bWithMic: %d.", __FUNCTION__, bWithMic);
	RETURN_FALSE_IF_ENGINE_NOT_INITIALIZED()

	if (IsPushCamera()) {
		PRINT_LOG(SimpleLogger::LOG_TYPE::L_INFO, "already push camera.");
		return true;
	}

	SetPushCamera(current_camera_.idx);
	ChannelMediaOptions op;
	op.publishAudioTrack = bWithMic;
	op.publishCameraTrack = true;
	op.clientRoleType = CLIENT_ROLE_TYPE::CLIENT_ROLE_BROADCASTER;
	int ret = rtc_engine_->updateChannelMediaOptions(op, camera_connId_);
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_INFO, "updateChannelMediaOptions, ret: %d.", ret);

	rtc_engine_->enableLocalVideo(true);

	if (bWithMic) {
		rtc_engine_->enableLocalAudio(true);
		is_publish_camera_audio_ = true;
	}

	if (ret == 0) {
		is_publish_camera_ = true;
	}

	rtc_engine_->startPreview();

	return ret ? false : true;
}

bool CAgoraManager::StopPushCamera() {
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_FUNC, "%s", __FUNCTION__);
	RETURN_FALSE_IF_ENGINE_NOT_INITIALIZED()

	if (!IsPushCamera()) {
		PRINT_LOG(SimpleLogger::LOG_TYPE::L_INFO, "already stop camera.");
		return true;
	}

	ChannelMediaOptions op;
	
	rtc_engine_->enableLocalVideo(false);
	if (is_publish_camera_audio_) {
		rtc_engine_->enableLocalAudio(false);
		op.publishAudioTrack = false;
		is_publish_camera_audio_ = false;
	}

	op.publishCameraTrack = false;
	op.clientRoleType = CLIENT_ROLE_TYPE::CLIENT_ROLE_AUDIENCE;
	int ret = rtc_engine_->updateChannelMediaOptions(op, camera_connId_);
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_INFO, "updateChannelMediaOptions, ret: %d.", ret);

	if (ret == 0) {
		is_publish_camera_ = false;
	}

	return (ret ? false : true);
}

bool CAgoraManager::IsPushCamera() {
	return is_publish_camera_;
}

void CAgoraManager::UpdatePushScreenConfig(int nPushW, int nPushH, int nPushFrameRate) {
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_FUNC, "%s: nPushW: %d, nPushH: %d, nPushFrameRate: %d.", __FUNCTION__, nPushW, nPushH, nPushFrameRate);
	RETURN_IF_ENGINE_NOT_INITIALIZED()

	param_.dimensions.width = nPushW;
	param_.dimensions.height = nPushH;
	param_.frameRate = nPushFrameRate;
	int ret = rtc_engine_->updateScreenCaptureParameters(param_);
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_INFO, "updateScreenCaptureParameters, ret: %d.", ret);
}

bool CAgoraManager::StartPushScreen(bool bWithMic, int nPushFps) {
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_FUNC, "%s: bWithMic: %d, nPushFps: %d.", __FUNCTION__, bWithMic, nPushFps);
	RETURN_FALSE_IF_ENGINE_NOT_INITIALIZED()

	if (IsPushScreen()) {
		PRINT_LOG(SimpleLogger::LOG_TYPE::L_INFO, "already start screen share.");
		return true;
	}

	param_.excludeWindowList = reinterpret_cast<agora::view_t *>(exclude_window_list_.data());
	param_.excludeWindowCount = exclude_window_list_.size();
	param_.frameRate = nPushFps;

	int ret = -1;
	int ret1 = -1;

	PRINT_LOG(SimpleLogger::LOG_TYPE::L_INFO, "encoder_config: width: %d, height: %d, framerate: %d, bitrate: %d.",
		param_.dimensions.width, param_.dimensions.height, param_.frameRate, param_.bitrate);

	if (share_win_) {
		ret = rtc_engine_->startScreenCaptureByWindowId((agora::view_t)share_win_, region_rect_, param_);
		PRINT_LOG(SimpleLogger::LOG_TYPE::L_INFO, "startScreenCaptureByWindowId, win: %d, region[x:%d,y:%d,w:%d,h:%d], ret: %d.",
			share_win_, region_rect_.x, region_rect_.y, region_rect_.width, region_rect_.height, ret);
	}
	else {
		PRINT_LOG(SimpleLogger::LOG_TYPE::L_INFO, "exclude_window_size: %d.", exclude_window_list_.size());
		for (auto win : exclude_window_list_) {
			PRINT_LOG(SimpleLogger::LOG_TYPE::L_INFO, "exclude_window_id: %d.", win);
		}
		ret = rtc_engine_->startScreenCaptureByScreenRect(screen_rect_, region_rect_, param_);
		PRINT_LOG(SimpleLogger::LOG_TYPE::L_INFO, "startScreenCaptureByScreenRect, screen[x:%d,y:%d,w:%d,h:%d], region[x:%d,y:%d,w:%d,h:%d], ret: %d.",
			share_win_, screen_rect_.x, screen_rect_.y, screen_rect_.width, screen_rect_.height,
			region_rect_.x, region_rect_.y, region_rect_.width, region_rect_.height, ret);
	}

	if (ret == 0) {
		ChannelMediaOptions op;
		op.publishScreenTrack = true;
		op.publishAudioTrack = bWithMic;
		op.clientRoleType = CLIENT_ROLE_TYPE::CLIENT_ROLE_BROADCASTER;

		if (bWithMic) {
			rtc_engine_->enableLocalAudio(true);
			is_publish_screen_audio_ = true;
		}

		ret1 = rtc_engine_->updateChannelMediaOptions(op, screen_connId_);
		PRINT_LOG(SimpleLogger::LOG_TYPE::L_INFO, "updateChannelMediaOptions, ret: %d.", ret1);

		is_publish_screen_ = true;
	}

	rtc_engine_->startPreview();

	return (ret || ret1) ? false : true;
}

void CAgoraManager::SetPushDesktop(int nScreenID,
	int x, int y, int w, int h) {
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_FUNC, "%s: nScreenID: %d, region[x:%d,y:%d,w:%d,h:%d].", __FUNCTION__,
		nScreenID, x, y, w, h);
	RETURN_IF_ENGINE_NOT_INITIALIZED()

	std::vector<DesktopInfo> desktops;
	GetDesktopList(desktops, 300, 300);

	if (nScreenID < 0 || nScreenID >= desktops.size()) {
		PRINT_LOG(SimpleLogger::LOG_TYPE::L_ERROR, "desktop_size: %d, screen_id: %d.", desktops.size(), nScreenID);
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
		int ret = rtc_engine_->updateScreenCaptureRegion(region_rect_);
		PRINT_LOG(SimpleLogger::LOG_TYPE::L_INFO, "updateScreenCaptureRegion: ret: %d.", ret);
	}
}

bool CAgoraManager::StopPushScreen() {
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_FUNC, "%s", __FUNCTION__);
	RETURN_FALSE_IF_ENGINE_NOT_INITIALIZED()

	if (!IsPushScreen()) {
		PRINT_LOG(SimpleLogger::LOG_TYPE::L_INFO, "already stop screen share.");
		return true;
	}

	rtc_engine_->stopScreenCapture();

	ChannelMediaOptions op;

	if (is_publish_screen_audio_) {
		rtc_engine_->enableLocalAudio(false);
		op.publishAudioTrack = false;
		is_publish_screen_audio_ = false;
		PRINT_LOG(SimpleLogger::LOG_TYPE::L_INFO, "enableLocalAudio(false).");
	}

	op.publishScreenTrack = false;
	op.clientRoleType = CLIENT_ROLE_TYPE::CLIENT_ROLE_AUDIENCE;
	int ret = rtc_engine_->updateChannelMediaOptions(op, screen_connId_);

	is_publish_screen_ = false;
	share_win_ = 0;

	PRINT_LOG(SimpleLogger::LOG_TYPE::L_INFO, "updateChannelMediaOptions, ret: %d.", ret);

	return ret ? false : true;
}

void CAgoraManager::SetPushWindow(HWND hwnd,
	int x, int y, int w, int h) {
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_FUNC, "%s: hwnd: %d, region[x:%d,y:%d,w:%d,h:%d].", __FUNCTION__,
		hwnd, x, y, w, h);

	share_win_ = hwnd;
	region_rect_.x = x;
	region_rect_.y = y;
	region_rect_.width = w;
	region_rect_.height = h;
}

void CAgoraManager::GetWindowList(std::vector<WindowInfo>& vWindows, int nThumbSizeW, int nThumbSizeH, int nIconSizeW, int nIconSizeH) {
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_FUNC, "%s: nThumbSizeW: %d, nThumbSizeH: %d, nIconSizeW: %d, nIconSizeH: %d.", __FUNCTION__,
		nThumbSizeW, nThumbSizeH, nIconSizeW, nIconSizeH);

	std::list<std::string> filters;
	auto win_list = app::utils::WindowEnumer::EnumAllWindows(filters, nThumbSizeW, nThumbSizeH, nIconSizeW, nIconSizeH);
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

			if (item->icon.data.size() > 0) {
				prog.iconBGRA.buffer = item->icon.data;
				prog.iconBGRA.length = item->icon.data.size();
			}
			else {
				continue;
			}
			
			prog.iconBGRA.width = item->icon.width;
			prog.iconBGRA.height = item->icon.height;

			if (item->thumb.data.size() > 0) {
				prog.thumbBGRA.buffer = item->thumb.data;
				prog.thumbBGRA.length = item->thumb.data.size();
			}
			else {
				continue;
			}
			
			prog.thumbBGRA.width = item->thumb.width;
			prog.thumbBGRA.height = item->thumb.height;
			vWindows.push_back(prog);
		}
	}
}

void CAgoraManager::GetDesktopList(std::vector<DesktopInfo>& vDesktop, int nThumbSizeW, int nThumbSizeH) {
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_FUNC, "%s: nThumbSizeW: %d, nThumbSizeH: %d.", __FUNCTION__, nThumbSizeW, nThumbSizeH);

	std::list<app::utils::WindowEnumer::MONITOR_INFO> desktops = app::utils::WindowEnumer::EnumAllMonitors(nThumbSizeW, nThumbSizeH);
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
		prog.thumbBGRA.buffer = it->thumb.data;
		prog.thumbBGRA.length = it->thumb.data.size();
		prog.thumbBGRA.width = it->thumb.width;
		prog.thumbBGRA.height = it->thumb.height;
		vDesktop.push_back(prog);
	}
}

void CAgoraManager::SetPushFilter(HWND* pFilterHwndList, int nFilterNum) {
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_FUNC, "%s: pFilterHwndList: %p, nFilterNum: %d.", __FUNCTION__,
		pFilterHwndList ? pFilterHwndList : 0, nFilterNum);

	if (!pFilterHwndList) {
		return;
	}

	exclude_window_list_.clear();
	for (int i = 0; i < nFilterNum; i++) {
		exclude_window_list_.push_back(pFilterHwndList[i]);
	}

	PRINT_LOG(SimpleLogger::LOG_TYPE::L_INFO, "exclude_window_size: %d.", exclude_window_list_.size());
	for (auto win : exclude_window_list_) {
		PRINT_LOG(SimpleLogger::LOG_TYPE::L_INFO, "exclude_window_id: %d.", win);
	}

	param_.excludeWindowList = reinterpret_cast<agora::view_t *>(exclude_window_list_.data());
	param_.excludeWindowCount = exclude_window_list_.size();

	int ret = rtc_engine_->updateScreenCaptureParameters(param_);
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_INFO, "updateScreenCaptureParameters, ret: %d.", ret);
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
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_FUNC, "%s: enable: %d.", __FUNCTION__, enable);
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
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_FUNC, "%s: bPause: %d.", __FUNCTION__, bPause);

	RETURN_IF_ENGINE_NOT_INITIALIZED()

	ChannelMediaOptions op;
	op.publishCameraTrack = (!bPause);
	int ret = rtc_engine_->updateChannelMediaOptions(op, camera_connId_);
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_INFO, "updateChannelMediaOptions, ret: %d.", ret);

	if (ret == 0) {
		is_mute_camera_ = bPause;
	}
}

bool CAgoraManager::IsPushCameraPause() {
	return is_mute_camera_;
}

void CAgoraManager::SetPushCameraAudioMute(bool bMute) {
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_FUNC, "%s: bMute: %d.", __FUNCTION__, bMute);
	RETURN_IF_ENGINE_NOT_INITIALIZED()

	int ret = rtc_engine_->muteLocalAudioStream(bMute);
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_INFO, "muteLocalAudioStream, ret: %d.", ret);
	if (ret == 0) {
		is_mute_mic_ = bMute;
	}
}

bool CAgoraManager::IsPushCameraAudioMute() {
	return is_mute_mic_;
}

void CAgoraManager::SetPushCamera(int nCamID) {
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_FUNC, "%s: nCamID: %d.", __FUNCTION__, nCamID);
	RETURN_IF_ENGINE_NOT_INITIALIZED()

	agora::rtc::IVideoDeviceManager* vdm = nullptr;
	rtc_engine_->queryInterface(agora::rtc::AGORA_IID_VIDEO_DEVICE_MANAGER,
		reinterpret_cast<void**>(&vdm));
	if (!vdm) {
		PRINT_LOG(SimpleLogger::LOG_TYPE::L_ERROR, "failed to get vdm!");
		return;
	}

	std::unique_ptr<agora::rtc::IVideoDeviceManager> video_device_manager(vdm);

	std::vector<CameraInfo> camera_list;
	GetCameraList(camera_list);
	for (int i = 0; i < camera_list.size(); i++) {
		if (nCamID == camera_list[i].idx) {
			video_device_manager->setDevice(camera_list[i].device_id.c_str());
			current_camera_ = CameraInfo{i, camera_list[i].device_id, ""};
			break;
		}
	}
}

void CAgoraManager::GetCameraList(std::vector<CameraInfo>& vCamera) {
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_FUNC, "%s", __FUNCTION__);
	RETURN_IF_ENGINE_NOT_INITIALIZED()

	agora::rtc::IVideoDeviceManager* vdm = nullptr;
	rtc_engine_->queryInterface(agora::rtc::AGORA_IID_VIDEO_DEVICE_MANAGER,
		reinterpret_cast<void**>(&vdm));
	if (!vdm) {
		PRINT_LOG(SimpleLogger::LOG_TYPE::L_ERROR, "failed to get vdm!");
		return;
	}

	std::unique_ptr<agora::rtc::IVideoDeviceManager> video_device_manager(vdm);
	std::unique_ptr<agora::rtc::IVideoDeviceCollection> vdc(video_device_manager->enumerateVideoDevices());

	int count = vdc->getCount();
	if (count <= 0) {
		PRINT_LOG(SimpleLogger::LOG_TYPE::L_WARN, "can not find any video device.");
		return;
	}

	char device_name_utf8[agora::rtc::MAX_DEVICE_ID_LENGTH] = { 0 };
	char device_id[agora::rtc::MAX_DEVICE_ID_LENGTH] = { 0 };
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
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_FUNC, "%s: nID.", __FUNCTION__, nID);
	RETURN_IF_ENGINE_NOT_INITIALIZED()

	agora::rtc::IAudioDeviceManager* adm = nullptr;
	rtc_engine_->queryInterface(agora::rtc::AGORA_IID_AUDIO_DEVICE_MANAGER,
		reinterpret_cast<void**>(&adm));
	if (!adm) {
		PRINT_LOG(SimpleLogger::LOG_TYPE::L_ERROR, "failed to get adm!");
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
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_FUNC, "%s", __FUNCTION__);
	RETURN_IF_ENGINE_NOT_INITIALIZED()

	agora::rtc::IAudioDeviceManager* adm = nullptr;
	rtc_engine_->queryInterface(agora::rtc::AGORA_IID_AUDIO_DEVICE_MANAGER,
		reinterpret_cast<void**>(&adm));
	if (!adm) {
		PRINT_LOG(SimpleLogger::LOG_TYPE::L_ERROR, "failed to get adm!");
		return;
	}

	std::unique_ptr<agora::rtc::IAudioDeviceManager> audio_device_manager(adm);
	std::unique_ptr<agora::rtc::IAudioDeviceCollection> adc(audio_device_manager->enumerateRecordingDevices());

	int count = adc->getCount();
	if (count <= 0) {
		PRINT_LOG(SimpleLogger::LOG_TYPE::L_WARN, "can not find any audio device.");
		return;
	}

	char device_name_utf8[agora::rtc::MAX_DEVICE_ID_LENGTH] = { 0 };
	char device_id[agora::rtc::MAX_DEVICE_ID_LENGTH] = { 0 };
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
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_FUNC, "%s: nVol: %d.", __FUNCTION__, nVol);
	RETURN_IF_ENGINE_NOT_INITIALIZED()

	int ret = rtc_engine_->adjustRecordingSignalVolume(nVol);
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_INFO, "adjustRecordingSignalVolume, ret: %d.", ret);
}

void CAgoraManager::SetSystemMicVolume(int nVol) {
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_FUNC, "%s: nVol: %d.", __FUNCTION__, nVol);
	RETURN_IF_ENGINE_NOT_INITIALIZED()

	agora::rtc::IAudioDeviceManager* adm = nullptr;
	rtc_engine_->queryInterface(agora::rtc::AGORA_IID_AUDIO_DEVICE_MANAGER,
		reinterpret_cast<void**>(&adm));
	if (!adm) {
		PRINT_LOG(SimpleLogger::LOG_TYPE::L_ERROR, "failed to get adm!");
		return;
	}

	std::unique_ptr<agora::rtc::IAudioDeviceManager> audio_device_manager(adm);
	int ret = audio_device_manager->setRecordingDeviceVolume(nVol);
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_INFO, "setRecordingDeviceVolume, ret: %d.", ret);
}

void CAgoraManager::SetPushSystemAudio(int nMode) {
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_FUNC, "%s: nMode: %d.", __FUNCTION__, nMode);

	if (nMode < PushSystemAudioOption_None || nMode > PushSystemAudioOption_Screen) {
		PRINT_LOG(SimpleLogger::LOG_TYPE::L_ERROR, "invalid system audio mode, mode: %d!", nMode);
		return;
	}

	if (nMode == current_recording_mode_) {
		PRINT_LOG(SimpleLogger::LOG_TYPE::L_INFO, "no need to change mode.");
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
		}
		else if (nMode == PushSystemAudioOption_Screen) {
			conn_id = screen_connId_;
		}
		rtc_engine_->enableLoopbackRecording(conn_id, true);
	}

	current_recording_mode_ = nMode;
}

void CAgoraManager::StartPlayer(agora::rtc::uid_t uid) {
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_FUNC, "%s: uid: %u.", __FUNCTION__, uid);
	RETURN_IF_ENGINE_NOT_INITIALIZED()

	int ret = rtc_engine_->muteRemoteVideoStream(uid, false);
	int ret1 = rtc_engine_->muteRemoteAudioStream(uid, false);
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_INFO, "muteRemoteVideoStream, ret: %d.", ret);
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_INFO, "muteRemoteAudioStream, ret1: %d.", ret1);
}

void CAgoraManager::StopPlayer(agora::rtc::uid_t uid) {
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_FUNC, "%s: uid: %u.", __FUNCTION__, uid);
	RETURN_IF_ENGINE_NOT_INITIALIZED()

	int ret = rtc_engine_->muteRemoteVideoStream(uid, true);
	int ret1 = rtc_engine_->muteRemoteAudioStream(uid, true);
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_INFO, "muteRemoteVideoStream, ret: %d.", ret);
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_INFO, "muteRemoteAudioStream, ret1: %d.", ret1);
}

void CAgoraManager::PausePlayer(agora::rtc::uid_t uid, bool bPause) {
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_FUNC, "%s: uid: %u, bPause: %d.", __FUNCTION__, uid, bPause);
	RETURN_IF_ENGINE_NOT_INITIALIZED()

	int ret = rtc_engine_->muteRemoteVideoStream(uid, bPause);
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_INFO, "muteRemoteVideoStream, ret: %d.", ret);
}

void CAgoraManager::MutePlayer(agora::rtc::uid_t uid, bool bMute) {
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_FUNC, "%s: uid: %u, bMute: %d.", __FUNCTION__, uid, bMute);
	RETURN_IF_ENGINE_NOT_INITIALIZED()

	int ret = rtc_engine_->muteRemoteAudioStream(uid, bMute);
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_INFO, "muteRemoteAudioStream, ret: %d.", ret);
}

void CAgoraManager::GetPlayerUID(std::vector<agora::rtc::uid_t>& uidList) {
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_FUNC, "%s", __FUNCTION__);

	std::lock_guard<std::mutex> lck (mtx_);
	RETURN_IF_ENGINE_NOT_INITIALIZED()

	for (auto it = users_in_channel_.begin(); it != users_in_channel_.end(); it++) {
		uidList.push_back(*it);
	}
}

void CAgoraManager::OnJoinChannelSuccess(const char* channel, uid_t uid, int elapsed, conn_id_t connId) {
	if (connId == DEFAULT_CONNECTION_ID) {
		camera_uid_ = uid;
		is_joined_ = true;
	}
	else {

	}
}

void CAgoraManager::SetPushScreenPause(bool bPause) {
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_FUNC, "%s, bPause: %d.", __FUNCTION__, bPause);
	RETURN_IF_ENGINE_NOT_INITIALIZED()

	ChannelMediaOptions op;
	op.publishScreenTrack = (!bPause);
	int ret = rtc_engine_->updateChannelMediaOptions(op, screen_connId_);
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_INFO, "updateChannelMediaOptions, ret: %d.", ret);

	if (ret == 0) {
		is_mute_screen_ = bPause;
	}
}

bool CAgoraManager::IsPushScreenPause() {
	return is_mute_screen_;
}

void CAgoraManager::SetPushScreenAudioMute(bool bMute) {
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_FUNC, "%s, bMute: %d.", __FUNCTION__, bMute);
	RETURN_IF_ENGINE_NOT_INITIALIZED()

	int ret = rtc_engine_->muteLocalAudioStream(bMute);
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_INFO, "muteLocalAudioStream, ret: %d.", ret);

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
		PRINT_LOG(SimpleLogger::LOG_TYPE::L_INFO, "OnUserJoined, uid: %u, elapsed: %d.", uid, elapsed);

		if (uid != screen_uid_ && uid != custom_uid_) {
			std::lock_guard<std::mutex> lck (mtx_);
			users_in_channel_.insert(uid);
		}
		else {
			rtc_engine_->muteRemoteAudioStream(uid, true);
			rtc_engine_->muteRemoteVideoStream(uid, true);
		}
	}
}

void CAgoraManager::OnUserOffline(uid_t uid, USER_OFFLINE_REASON_TYPE reason, conn_id_t connId) {
	if (connId == DEFAULT_CONNECTION_ID) {
		PRINT_LOG(SimpleLogger::LOG_TYPE::L_INFO, "OnUserOffline, uid: %u, reason: %d.", uid, reason);

		std::lock_guard<std::mutex> lck (mtx_);
		users_in_channel_.erase(uid);
	}
}

void CAgoraManager::onError(int err, const char* msg, conn_id_t connId) {
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_INFO, "onError, err: %d, msg: %s, connId: %d.", err, msg, connId);
}

void CAgoraManager::onConnectionStateChanged(CONNECTION_STATE_TYPE state, CONNECTION_CHANGED_REASON_TYPE reason, conn_id_t connId) {
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_INFO, "onConnectionStateChanged, state: %d, reason: %d, connId: %d.", state, reason, connId);
}

void CAgoraManager::onMediaDeviceChanged(int deviceType, conn_id_t connId) {
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_INFO, "onMediaDeviceChanged, deviceType: %d, connId: %d.", deviceType, connId);

	if (deviceType == 3) {
		std::vector<CAgoraManager::CameraInfo> camera_list;
		GetCameraList(camera_list);
		for (int i = 0; i < camera_list.size(); i++) {
			if (current_camera_.device_id == camera_list[i].device_id) {
				current_camera_.idx = i;
				StartPushCamera();
				return;
			}
		}
		StopPushCamera();
	}
}

void CAgoraManager::onLocalVideoStateChanged(LOCAL_VIDEO_STREAM_STATE state, LOCAL_VIDEO_STREAM_ERROR error, conn_id_t connId) {
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_INFO, "onLocalVideoStateChanged, deviceType: %d, connId: %d.", state, error, connId);
}

void CAgoraManager::onLocalAudioStateChanged(LOCAL_AUDIO_STREAM_STATE state, LOCAL_AUDIO_STREAM_ERROR error, conn_id_t connId) {
	printf("[I]: onLocalAudioStateChanged, state: %d, error: %d, connId: %d\n", state, error, connId);
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_INFO, "onLocalVideoStateChanged, deviceType: %d, connId: %d.", state, error, connId);
}

void CAgoraManager::onRemoteVideoStateChanged(uid_t uid, REMOTE_VIDEO_STATE state, REMOTE_VIDEO_STATE_REASON reason, int elapsed, conn_id_t connId) {
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_INFO, "onRemoteVideoStateChanged, uid: %u, state: %d, reason: %d, elapsed: %d, connId: %d.", uid, state, reason, elapsed, connId);
}

void CAgoraManager::onRemoteAudioStateChanged(uid_t uid, REMOTE_AUDIO_STATE state, REMOTE_AUDIO_STATE_REASON reason, int elapsed, conn_id_t connId) {
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_INFO, "onRemoteAudioStateChanged, uid: %u, state: %d, reason: %d, elapsed: %d, connId: %d.", uid, state, reason, elapsed, connId);
}

void CAgoraManager::onFirstLocalVideoFramePublished(int elapsed, conn_id_t connId) {
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_INFO, "onFirstLocalVideoFramePublished, elapsed: %d, connId: %d.", elapsed, connId);
}

void CAgoraManager::onFirstLocalAudioFramePublished(int elapsed, conn_id_t connId) {
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_INFO, "onFirstLocalAudioFramePublished, elapsed: %d, connId: %d.", elapsed, connId);
}
	
void CAgoraManager::ResetStates() {
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_FUNC, "%s", __FUNCTION__);

	is_joined_ = false;
	is_publish_camera_ = false;
	is_publish_screen_ = false;
	is_publish_custom_ = false;
	is_publish_camera_audio_ = false;
	is_publish_screen_audio_ = false;
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
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_FUNC, "%s", __FUNCTION__);
	RETURN_FALSE_IF_ENGINE_NOT_INITIALIZED()

	ChannelMediaOptions op;
	op.publishCustomAudioTrack = true;
	op.publishCustomVideoTrack = true;
	op.clientRoleType = CLIENT_ROLE_TYPE::CLIENT_ROLE_BROADCASTER;
	int ret = rtc_engine_->updateChannelMediaOptions(op, custom_connId_);
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_INFO, "updateChannelMediaOptions, ret: %d.", ret);

	if (ret == 0) {
		is_publish_custom_ = true;
	}

	ret = rtc_engine_->startPreview();
	return ret == 0 ? true : false;
}

bool CAgoraManager::StopPushCustom()
{
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_FUNC, "%s", __FUNCTION__);
	RETURN_FALSE_IF_ENGINE_NOT_INITIALIZED()
	
	ChannelMediaOptions op;
	op.publishCustomAudioTrack = false;
	op.publishCustomVideoTrack = false;
	op.clientRoleType = CLIENT_ROLE_TYPE::CLIENT_ROLE_AUDIENCE;
	int ret = rtc_engine_->updateChannelMediaOptions(op, custom_connId_);
	PRINT_LOG(SimpleLogger::LOG_TYPE::L_INFO, "updateChannelMediaOptions, ret: %d.", ret);

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

bool CAgoraManager::PushAudioFrame(unsigned char * pData, int nbSamples, long lSampleRate, int nChannel, long long ms)
{
	RETURN_FALSE_IF_ENGINE_NOT_INITIALIZED()

	agora::media::IAudioFrameObserver::AudioFrame frame;
	frame.type = agora::media::IAudioFrameObserver::FRAME_TYPE_PCM16;
	frame.avsync_type = 0;
	frame.buffer = pData;
	frame.channels = nChannel;
	frame.bytesPerSample = BYTES_PER_SAMPLE::TWO_BYTES_PER_SAMPLE;
	frame.samplesPerSec = lSampleRate;
	frame.samplesPerChannel = nbSamples;
	frame.renderTimeMs = ms;

	int ret = media_engine_->pushAudioFrame(agora::media::AUDIO_RECORDING_SOURCE, &frame, false, 0, custom_connId_);
	return ret == 0 ? true : false;
}

