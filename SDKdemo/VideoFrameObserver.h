#pragma once
#include <stdint.h>
#include <stdio.h>
#include <atomic>
#include <map>
#include <mutex>
#include <thread>
#include <vector>
#include "IAgoraMediaEngine.h"

class VideoFrameObserver : public agora::media::IVideoFrameObserver {
 public:
  VideoFrameObserver();
  virtual ~VideoFrameObserver();

  bool init();
  void uninit();

  bool GetPlayerImageSize(agora::rtc::uid_t uid, int& nRetW, int& nRetH);
  bool getPlayerImage(agora::rtc::uid_t uid, BYTE* pData, int& nRetW, int& nRetH);

  bool GetCameraImageSize(int& nRetW, int& nRetH);
  bool getCameraImage(BYTE* pData, int& nRetW, int& nRetH);
	
  bool GetScreenImageSize(int& nRetW, int& nRetH);
  bool getScreenImage(BYTE* pData, int& nRetW, int& nRetH);

 protected:
  virtual agora::media::base::VIDEO_PIXEL_FORMAT getVideoPixelFormatPreference() { return m_videoFrameType; }
  virtual bool onCaptureVideoFrame(VideoFrame& videoFrame);
  virtual bool onScreenCaptureVideoFrame(VideoFrame& videoFrame);
  virtual bool onRenderVideoFrame(agora::rtc::uid_t uid, agora::rtc::conn_id_t connectionId,
                                  VideoFrame& videoFrame);

  virtual bool onSecondaryCameraCaptureVideoFrame(VideoFrame& videoFrame) { return false; }
  virtual bool onMediaPlayerVideoFrame(VideoFrame& videoFrame, int mediaPlayerId) { return false; }
  virtual bool onSecondaryScreenCaptureVideoFrame(VideoFrame& videoFrame) { return false; }
  virtual bool onTranscodedVideoFrame(VideoFrame& videoFrame) { return false; }

  void setVideoPixelFormatPreference(agora::media::base::VIDEO_PIXEL_FORMAT type) { m_videoFrameType = type; }

private:
  bool CacheVideoFrame(agora::rtc::uid_t uid, VideoFrame& videoFrame);

 private:

  VideoFrame m_currentCameraVideoFrame;
  VideoFrame m_currentScreenVideoFrame;
  std::map<agora::rtc::uid_t, VideoFrame> m_renderVideoFrameCache;

  std::mutex m_mt;

  agora::media::base::VIDEO_PIXEL_FORMAT m_videoFrameType = agora::media::base::VIDEO_PIXEL_RGBA;

  std::atomic_bool m_running = false;

  int buffer_size_ = 0;
};
