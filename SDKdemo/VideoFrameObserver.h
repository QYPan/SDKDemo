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

  bool init(int role);
  void uninit();

  void startRenderThread();

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
  int selectFileToDump(unsigned int uid, int w, int h, agora::media::base::VIDEO_PIXEL_FORMAT frameType, FILE*& f);
  std::string jointFileName(unsigned int uid, int w, int h, agora::media::base::VIDEO_PIXEL_FORMAT frameType);

private:
  bool onVideoFrame(agora::rtc::uid_t uid, VideoFrame& videoFrame);
  bool CacheVideoFrame(agora::rtc::uid_t uid, VideoFrame& videoFrame);

 private:
  struct FrameInfo {
    FrameInfo(FILE* flp, const VideoFrame& f) : file(flp), frame(f) {}
    ~FrameInfo() {}
    FILE* file;
    VideoFrame frame;
  };

  VideoFrame m_currentCameraVideoFrame;
  VideoFrame m_currentScreenVideoFrame;
  VideoFrame m_currentRenderVideoFrame;

  std::map<agora::rtc::uid_t, VideoFrame> m_renderVideoFrameCache;

  std::thread m_renderThread;
  std::vector<FrameInfo> m_frameQueue;
  std::map<std::string, FILE*> m_fileMap;
  std::mutex m_mt;
  agora::media::base::VIDEO_PIXEL_FORMAT m_videoFrameType = agora::media::base::VIDEO_PIXEL_RGBA;

  int64_t m_fileSizeLimit = 0;
  std::atomic_bool m_running = false;
  int m_role = 1;

  int buffer_size_ = 0;
};
