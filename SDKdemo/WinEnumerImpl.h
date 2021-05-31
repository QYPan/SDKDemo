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

  static std::list<MONITOR_INFO> EnumAllMonitors();

  static MONITOR_INFO GetMonitorInfoByIndex(int index);


  typedef struct {
    HWND hwnd;
    std::wstring window_name;
    std::wstring class_name;
    std::wstring module_name;
  }WINDOW_INFO;

  static std::map<std::wstring,std::list<WINDOW_INFO>> EnumAllWindows(const std::list<std::wstring>& filters);
};

}
}