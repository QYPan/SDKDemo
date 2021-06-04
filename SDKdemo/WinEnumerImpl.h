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
	  std::string sourceName;///< ���ڱ������ƣ�UTF8 ����
	  std::string moduleName;
	  bool   isMinimizeWindow;///< �Ƿ�Ϊ��С������
	  int    x; //����λ��x
	  int    y; //����λ��y
	  int    width;//���ڿ��
	  int    height;//���ڸ߶�
	  HBITMAP hBitmap;
	  HBITMAP hIcon;
  }WINDOW_INFO;

  static std::list<MONITOR_INFO> EnumAllMonitors();

  static MONITOR_INFO GetMonitorInfoByIndex(int index);

  static std::map<std::string, std::list<WindowEnumer::WINDOW_INFO>> EnumAllWindows(const std::list<std::string>& filters);
};

}
}