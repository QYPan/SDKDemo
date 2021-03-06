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

  int getPlayerImageW(agora::rtc::uid_t uid);
  int getPlayerImageH(agora::rtc::uid_t uid);
  bool getPlayerImage(agora::rtc::uid_t uid, BYTE* pData, int& nRetW, int& nRetH);

  int getCameraImageW();
  int getCameraImageH();
  bool getCameraImage(BYTE* pData, int& nRetW, int& nRetH);
	
  int getScreenImageW();
  int getScreenImageH();
  bool getScreenImage(BYTE* pData, int& nRetW, int& nRetH);

 protected:
  virtual agora::media::base::VIDEO_PIXEL_FORMAT getVideoPixelFormatPreference() { return m_videoFrameType; }
  virtual bool onCaptureVideoFrame(VideoFrame& videoFrame);
  virtual bool onScreenCaptureVideoFrame(VideoFrame& videoFrame);
  virtual bool onRenderVideoFrame(agora::rtc::uid_t uid, agora::rtc::conn_id_t connectionId,
                                  VideoFrame& videoFrame);

  virtual bool onMediaPlayerVideoFrame(VideoFrame& videoFrame, int mediaPlayerId) { return false; }

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
