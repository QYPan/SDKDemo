#include "stdafx.h"

#include "VideoFrameObserver.h"

#include <fstream>

VideoFrameObserver::VideoFrameObserver() {}

bool VideoFrameObserver::init(int role) {
  m_role = role;

  m_fileSizeLimit = int64_t(1) * 1024 * 1024 * 1024;
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
  if (m_renderThread.joinable()) {
    m_renderThread.join();
  }

  for (auto it = m_fileMap.begin(); it != m_fileMap.end(); it++) {
    if (it->second) {
      fclose(it->second);
      it->second = NULL;
    }
  }

  m_fileMap.clear();

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

//agora::media::base::VideoFrame VideoFrameObserver::getCurrentVideoFrame() {
//
//}

void VideoFrameObserver::startRenderThread() {
  if (m_running) {
    return;
  }

  m_running = true;

  m_renderThread = std::thread([this] {
    while (m_running) {
      std::vector<FrameInfo> tmpFrameQueue;
      {
        std::lock_guard<std::mutex> lck(m_mt);
        tmpFrameQueue.swap(m_frameQueue);
      }

      for (auto& f : tmpFrameQueue) {
        FILE* fp = f.file;

        VideoFrame& videoFrame = f.frame;

        if (fp) {
          auto file_pos = ftell(fp);
          if (file_pos >= 0 && file_pos <= m_fileSizeLimit) {
            fwrite(videoFrame.yBuffer, 1, videoFrame.yStride * videoFrame.height, fp);
            if(videoFrame.type != agora::media::base::VIDEO_PIXEL_RGBA && videoFrame.type != agora::media::base::VIDEO_PIXEL_BGRA) {
              fwrite(videoFrame.uBuffer, 1, videoFrame.uStride * videoFrame.height / 2, fp);
              fwrite(videoFrame.vBuffer, 1, videoFrame.vStride * videoFrame.height / 2, fp);
            }
          }
        }

        if (videoFrame.yBuffer) {
          delete[] static_cast<uint8_t*>(videoFrame.yBuffer);
          videoFrame.yBuffer = nullptr;
        }
        if (videoFrame.uBuffer) {
          delete[] static_cast<uint8_t*>(videoFrame.uBuffer);
          videoFrame.uBuffer = nullptr;
        }
        if (videoFrame.vBuffer) {
          delete[] static_cast<uint8_t*>(videoFrame.vBuffer);
          videoFrame.vBuffer = nullptr;
        }
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
  });
}

std::string VideoFrameObserver::jointFileName(unsigned int uid, int w, int h,
                                              agora::media::base::VIDEO_PIXEL_FORMAT frameType) {
  char buffer[MAX_PATH] = {0};
  std::string role_str = (m_role == 1 ? "\\broadcaster" : "\\audience");
  std::string frame_type;

  switch(frameType) {
  case agora::media::base::VIDEO_PIXEL_I420: frame_type = "i420"; break;
  case agora::media::base::VIDEO_PIXEL_I422: frame_type = "i422"; break;
  case agora::media::base::VIDEO_PIXEL_RGBA: frame_type = "rgba"; break;
  case agora::media::base::VIDEO_PIXEL_BGRA: frame_type = "bgra"; break;
  default: frame_type = "unknow";
  }

  sprintf(buffer, "%s_uid_%u_%d_%d_%s.yuv", role_str.c_str(), uid, w, h, frame_type.c_str());

  return std::string(buffer);
}

int VideoFrameObserver::selectFileToDump(unsigned int uid, int w, int h,
                                         agora::media::base::VIDEO_PIXEL_FORMAT frameType,
                                         FILE*& f) {
  char buffer[MAX_PATH];
  ::GetModuleFileNameA(NULL, buffer, MAX_PATH);
  std::string::size_type pos = std::string(buffer).find_last_of("\\/");
  std::string file_path = std::string(buffer).substr(0, pos);
  std::string file_name = jointFileName(uid, w, h, frameType);

  auto it = m_fileMap.find(file_name);
  if (it != m_fileMap.end() && it->second) {
    auto file_pos = ftell(it->second);
    if (file_pos >= 0 && file_pos <= m_fileSizeLimit) {
      f = it->second;
      return 2;
    }
    return 0;
  }

  std::string filePath = file_path + file_name;

  m_fileMap[file_name] = fopen(filePath.c_str(), "wb");
  if (m_fileMap[file_name]) {
    f = m_fileMap[file_name];
    return 1;
  }

  return 0;
}

bool VideoFrameObserver::onVideoFrame(agora::rtc::uid_t uid, VideoFrame& videoFrame) {
  //assert(videoFrame.yStride == videoFrame.width);
  //assert(videoFrame.uStride == (videoFrame.width + 1) / 2);
  //assert(videoFrame.vStride == (videoFrame.width + 1) / 2);

  FILE* flp = NULL;
  if (!selectFileToDump(uid, videoFrame.width, videoFrame.height, videoFrame.type, flp)) {
    return false;
  }

  FrameInfo frameInfo(flp, videoFrame);

  int uSize = 0;
  int vSize = 0;

  if (videoFrame.type == agora::media::base::VIDEO_PIXEL_I422) {
    uSize = videoFrame.uStride * videoFrame.height;
    vSize = videoFrame.vStride * videoFrame.height;
  } else if (videoFrame.type == agora::media::base::VIDEO_PIXEL_I420) {
    uSize = videoFrame.uStride * videoFrame.height / 2;
    vSize = videoFrame.vStride * videoFrame.height / 2;
  }

  frameInfo.frame.yBuffer = new uint8_t[videoFrame.yStride * videoFrame.height];
  memcpy(frameInfo.frame.yBuffer, videoFrame.yBuffer, videoFrame.yStride * videoFrame.height);

  if (videoFrame.type != agora::media::base::VIDEO_PIXEL_RGBA && videoFrame.type != agora::media::base::VIDEO_PIXEL_BGRA) {
    frameInfo.frame.uBuffer = new uint8_t[videoFrame.uStride * videoFrame.height];
    memcpy(frameInfo.frame.uBuffer, videoFrame.uBuffer, uSize);

    frameInfo.frame.vBuffer = new uint8_t[videoFrame.vStride * videoFrame.height];
    memcpy(frameInfo.frame.vBuffer, videoFrame.vBuffer, vSize);
  }

  std::lock_guard<std::mutex> lck(m_mt);
  m_frameQueue.push_back(frameInfo);
  return true;
}

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
  // return onVideoFrame(0, videoFrame);
  return CacheVideoFrame(0, videoFrame);
}

bool VideoFrameObserver::onScreenCaptureVideoFrame(VideoFrame& videoFrame) {
  // return onVideoFrame(1, videoFrame);
  return CacheVideoFrame(1, videoFrame);
}

bool VideoFrameObserver::onRenderVideoFrame(agora::rtc::uid_t uid, agora::rtc::conn_id_t connectionId,
                                            VideoFrame& videoFrame) {
  if (connectionId == agora::rtc::DEFAULT_CONNECTION_ID) {
    // return onVideoFrame(uid, videoFrame);
    return CacheVideoFrame(uid, videoFrame);
  }
}
