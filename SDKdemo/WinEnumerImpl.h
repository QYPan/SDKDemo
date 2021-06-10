#pragma once

#include <list>
#include <map>
#include <windows.h>
#include <string>
#include <vector>

namespace app {
namespace utils {

class WindowEnumer {
public:

	typedef struct {
		int width;
		int height;
		std::vector<BYTE> data;
	}IMAGE_INFO;

  typedef struct {
    std::wstring name;
    RECT rc;
    bool is_primary;
    int index;
	IMAGE_INFO thumb;
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
	  IMAGE_INFO thumb;
	  IMAGE_INFO icon;
  }WINDOW_INFO;

  enum RENDER_TYPE {
	  RENDER_TYPE_SCALE,
	  RENDER_TYPE_FILL,
  };

  typedef struct {
	  std::list<WindowEnumer::MONITOR_INFO> monitors;
	  RENDER_TYPE renderType;
	  int maxWidth;
	  int maxHeight;
	  int fillWidth;
	  int fillHeight;
  }WIN_MONITORS;

  static std::list<MONITOR_INFO> EnumAllMonitors(int fillWidth, int fillHeight);

  static MONITOR_INFO GetMonitorInfoByIndex(int index);

  static std::map<std::string, std::list<WindowEnumer::WINDOW_INFO>> EnumAllWindows(const std::list<std::string>& filters);
};

}
}