#pragma once

#include <list>
#include <map>
#include <windows.h>
#include <string>

namespace app {
namespace utils {

class WindowEnumer {
public:

  typedef struct {
    std::wstring name;
    RECT rc;
    bool is_primary;
    int index;
  }MONITOR_INFO;

  typedef struct {
	  HWND sourceId;
	  std::string sourceName;///< 窗口标题名称，UTF8 编码
	  std::string moduleName;
	  bool   isMinimizeWindow;///< 是否为最小化窗口
	  int    x; //窗口位置x
	  int    y; //窗口位置y
	  int    width;//窗口宽度
	  int    height;//窗口高度
	  HBITMAP hBitmap;
	  HBITMAP hIcon;
  }WINDOW_INFO;

  static std::list<MONITOR_INFO> EnumAllMonitors();

  static MONITOR_INFO GetMonitorInfoByIndex(int index);

  static std::map<std::string, std::list<WindowEnumer::WINDOW_INFO>> EnumAllWindows(const std::list<std::string>& filters);
};

}
}