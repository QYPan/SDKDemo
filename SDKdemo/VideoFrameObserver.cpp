#include "stdafx.h"

#include "VideoFrameObserver.h"

#include <fstream>

VideoFrameObserver::VideoFrameObserver() {}

bool VideoFrameObserver::init() {
  m_running = false;

  buffer_size_ = 4 * 2000 * 2000;

  m_currentCameraVideoFrame.yBuffer = new uint8_t[buffer_size_];
  m_currentScreenVideoFrame.yBuffer = new uint8_t[buffer_size_];

  m_currentCameraVideoFrame.width = 0;
  m_currentCameraVideoFrame.height = 0;

  m_currentScreenVideoFrame.width = 0;
  m_currentScreenVideoFrame.height = 0;

  return true;
}

void VideoFrameObserver::uninit() {
  m_running = false;

  if (m_currentCameraVideoFrame.yBuffer) {
    // just use yBuffer
    delete m_currentCameraVideoFrame.yBuffer;
    m_currentCameraVideoFrame.yBuffer = nullptr;
  }

  if (m_currentScreenVideoFrame.yBuffer) {
    // just use yBuffer
    delete m_currentScreenVideoFrame.yBuffer;
    m_currentScreenVideoFrame.yBuffer = nullptr;
  }

  for (auto it = m_renderVideoFrameCache.begin(); it != m_renderVideoFrameCache.end(); it++) {
    // just use yBuffer
    if (it->second.yBuffer) {
        delete it->second.yBuffer;
        it->second.yBuffer = nullptr;
    }
  }

  m_renderVideoFrameCache.clear();
}

VideoFrameObserver::~VideoFrameObserver() { uninit(); }

int VideoFrameObserver::getPlayerImageW(agora::rtc::uid_t uid) {
  std::lock_guard<std::mutex> lck(m_mt);
  auto it = m_renderVideoFrameCache.find(uid);
  if (it != m_renderVideoFrameCache.end()) {
    return it->second.width;
  }

  return 0;
}

int VideoFrameObserver::getPlayerImageH(agora::rtc::uid_t uid) {
  std::lock_guard<std::mutex> lck(m_mt);
  auto it = m_renderVideoFrameCache.find(uid);
  if (it != m_renderVideoFrameCache.end()) {
    return it->second.height;
  }

  return 0;
}

bool VideoFrameObserver::getPlayerImage(agora::rtc::uid_t uid, BYTE* pData, int& nRetW, int& nRetH) {
  std::lock_guard<std::mutex> lck(m_mt);
  auto it = m_renderVideoFrameCache.find(uid);
  if (it == m_renderVideoFrameCache.end()) {
    return false;
  }

  nRetW = it->second.width;
  nRetH = it->second.height;
  int len = nRetW * nRetH * 4;

  if (!pData) {
    // invoker will manager the buffer
    pData = new BYTE[len];
  }

  memset(pData, 0, len);
  memcpy(pData, it->second.yBuffer, len);

  return true;
}

int VideoFrameObserver::getCameraImageW() {
  std::lock_guard<std::mutex> lck(m_mt);
  return m_currentCameraVideoFrame.width;
}

int VideoFrameObserver::getCameraImageH() {
  std::lock_guard<std::mutex> lck(m_mt);
  return m_currentCameraVideoFrame.height;
}

bool VideoFrameObserver::getCameraImage(BYTE* pData, int& nRetW, int& nRetH) {
  std::lock_guard<std::mutex> lck(m_mt);
  if (m_currentCameraVideoFrame.width == 0 || m_currentCameraVideoFrame.height == 0) {
    return false;
  }

  nRetW = m_currentCameraVideoFrame.width;
  nRetH = m_currentCameraVideoFrame.height;
  int len = nRetW * nRetH * 4;

  if (!pData) {
    // invoker will manager the buffer
    pData = new BYTE[len];
  }

  memset(pData, 0, len);
  memcpy(pData, m_currentCameraVideoFrame.yBuffer, len);

  return true;
}
	
int VideoFrameObserver::getScreenImageW() {
  std::lock_guard<std::mutex> lck(m_mt);
  return m_currentScreenVideoFrame.width;
}

int VideoFrameObserver::getScreenImageH() {
  std::lock_guard<std::mutex> lck(m_mt);
  return m_currentScreenVideoFrame.height;
}

bool VideoFrameObserver::getScreenImage(BYTE* pData, int& nRetW, int& nRetH) {
  std::lock_guard<std::mutex> lck(m_mt);
  if (m_currentScreenVideoFrame.width == 0 || m_currentScreenVideoFrame.height == 0) {
    return false;
  }

  nRetW = m_currentScreenVideoFrame.width;
  nRetH = m_currentScreenVideoFrame.height;
  int len = nRetW * nRetH * 4;

  if (!pData) {
    // invoker will manager the buffer
    pData = new BYTE[len];
  }

  memset(pData, 0, len);
  memcpy(pData, m_currentScreenVideoFrame.yBuffer, len);

  return true;
}

bool VideoFrameObserver::CacheVideoFrame(agora::rtc::uid_t uid, VideoFrame& videoFrame) {
  if (videoFrame.type != agora::media::base::VIDEO_PIXEL_RGBA) {
    return false;
  }

  std::lock_guard<std::mutex> lck(m_mt);

  if (uid == 0) {
    if (m_currentCameraVideoFrame.yBuffer) {
      m_currentCameraVideoFrame.yStride = videoFrame.yStride;
      m_currentCameraVideoFrame.width = videoFrame.width;
      m_currentCameraVideoFrame.height = videoFrame.height;
      memset(m_currentCameraVideoFrame.yBuffer, 0, buffer_size_);
      memcpy(m_currentCameraVideoFrame.yBuffer, videoFrame.yBuffer, videoFrame.yStride * videoFrame.height);
    }
  } else if (uid == 1) {
    if (m_currentScreenVideoFrame.yBuffer) {
      m_currentScreenVideoFrame.yStride = videoFrame.yStride;
      m_currentScreenVideoFrame.width = videoFrame.width;
      m_currentScreenVideoFrame.height = videoFrame.height;
      memset(m_currentScreenVideoFrame.yBuffer, 0, buffer_size_);
      memcpy(m_currentScreenVideoFrame.yBuffer, videoFrame.yBuffer, videoFrame.yStride * videoFrame.height);
    }
  } else {
    if (m_renderVideoFrameCache.find(uid) == m_renderVideoFrameCache.end()) {
      VideoFrame frame;
      frame.yBuffer = new uint8_t[buffer_size_];
      m_renderVideoFrameCache[uid] = frame;
    }

    VideoFrame& frame = m_renderVideoFrameCache[uid];

    if (frame.yBuffer) {
      frame.yStride = videoFrame.yStride;
      frame.width = videoFrame.width;
      frame.height = videoFrame.height;
      memset(frame.yBuffer, 0, buffer_size_);
      memcpy(frame.yBuffer, videoFrame.yBuffer, videoFrame.yStride * videoFrame.height);
    }
  }
}

bool VideoFrameObserver::onCaptureVideoFrame(VideoFrame& videoFrame) {
  return CacheVideoFrame(0, videoFrame);
}

bool VideoFrameObserver::onScreenCaptureVideoFrame(VideoFrame& videoFrame) {
  return CacheVideoFrame(1, videoFrame);
}

bool VideoFrameObserver::onRenderVideoFrame(agora::rtc::uid_t uid, agora::rtc::conn_id_t connectionId,
                                            VideoFrame& videoFrame) {
  if (connectionId == agora::rtc::DEFAULT_CONNECTION_ID) {
    return CacheVideoFrame(uid, videoFrame);
  }
}
