#include "stdafx.h"
#include "WinEnumerImpl.h"

#include <windows.h>
#include <dwmapi.h>
#include <Psapi.h>
#include <tchar.h>
#include <algorithm>

#include <atlbase.h>
#include <atlstr.h>

#include "process.h"
#include "SimpleWindow.h"
#include "MagnificationCapture.h"
#include "TaskWindow.h"

using namespace app::utils;



namespace app {
namespace utils {

BOOL WINAPI MonitorEnumCallback(HMONITOR monitor,
  HDC hdc,
  LPRECT lprc,
  LPARAM data) {

  MONITORINFOEX info_ex;
  info_ex.cbSize = sizeof(MONITORINFOEX);

  GetMonitorInfo(monitor, &info_ex);

  // https://jira.agoralab.co/browse/CSD-26297
  // mirror mode or non-active do not return
  if (info_ex.dwFlags == DISPLAY_DEVICE_MIRRORING_DRIVER)
    return true;

  WindowEnumer::WIN_MONITORS* winMonitors = (WindowEnumer::WIN_MONITORS*)data;
  auto& monitors = winMonitors->monitors;

  WindowEnumer::MONITOR_INFO info;
  info.index = monitors.size();
  info.rc = info_ex.rcMonitor;
  info.name = info_ex.szDevice;
  info.is_primary = info_ex.dwFlags & MONITORINFOF_PRIMARY;

  if (winMonitors->renderType == WindowEnumer::RENDER_TYPE_FILL) {
	  info.thumb.width = winMonitors->fillWidth;
	  info.thumb.height = winMonitors->fillHeight;
	  GetDesktopAREAData(info.rc, info.thumb.data,
		  winMonitors->fillWidth, winMonitors->fillHeight);
  }
  else {
	  int realWidth = info.rc.right - info.rc.left;
	  int realHeight = info.rc.bottom - info.rc.top;

	  int outWidth = realWidth;
	  int outHeight = realHeight;

	  if (outWidth > winMonitors->maxWidth) {
		  outWidth = winMonitors->maxWidth;
		  outHeight = 1.0 * outWidth * realHeight / realWidth;
	  }

	  if (outHeight > winMonitors->maxHeight) {
		  outHeight = winMonitors->maxHeight;
		  outWidth = 1.0 * outHeight * realWidth / realHeight;
	  }

	  info.thumb.width = outWidth;
	  info.thumb.height = outHeight;
	  GetDesktopAREAData(info.rc,info.thumb.data, outWidth, outHeight);
  }

  monitors.emplace_back(info);

  return true;
}

std::list<WindowEnumer::MONITOR_INFO> WindowEnumer::EnumAllMonitors(int fillWidth, int fillHeight)
{
  WIN_MONITORS winMonitors;
  winMonitors.renderType = WindowEnumer::RENDER_TYPE_FILL;
  winMonitors.fillWidth = fillWidth;
  winMonitors.fillHeight = fillHeight;

  ::EnumDisplayMonitors(NULL, NULL, MonitorEnumCallback, (LPARAM)&winMonitors);

  return winMonitors.monitors;
}

WindowEnumer::MONITOR_INFO WindowEnumer::GetMonitorInfoByIndex(int index)
{
	const int FILL_WIDTH = 600;
	const int FILL_HEIGHT = 400;
  auto monitors = EnumAllMonitors(FILL_WIDTH, FILL_HEIGHT);
  for (auto monitor : monitors) {
    if (monitor.index == index)
      return monitor;
  }

  return { L"",{0,0},false,-1 };
}

BOOL IsInvisibleWin10BackgroundAppWindow(HWND hWnd) {

  HRESULT(__stdcall * pDwmGetWindowAttribute) (HWND hwnd, DWORD dwAttribute, PVOID pvAttribute, DWORD cbAttribute) = NULL;
  HINSTANCE hDll = LoadLibrary(L"Dwmapi.dll");
  if (hDll != NULL) {
    pDwmGetWindowAttribute = (HRESULT(__stdcall*)(
      HWND hwnd, DWORD dwAttribute, PVOID pvAttribute,
      DWORD cbAttribute))GetProcAddress(hDll, "DwmGetWindowAttribute");
    int CloakedVal = 0;
    HRESULT hRes = pDwmGetWindowAttribute(hWnd, 14 /*DWMWA_CLOAKED*/,
      &CloakedVal, sizeof(CloakedVal));
    if (hRes != S_OK) {
      CloakedVal = 0;
    }
    return CloakedVal ? true : false;
  }
  return false;
}



BOOL WINAPI WindowEnumCallback(HWND hwnd,
  LPARAM data) {

  do {
    if (!::IsWindowVisible(hwnd))
      break;

	TCHAR class_name[MAX_PATH]{ 0 };
	::GetClassName(hwnd, class_name, MAX_PATH);

	TCHAR window_name[MAX_PATH]{ 0 };
	::GetWindowText(hwnd, window_name, MAX_PATH);

    if (IsInvisibleWin10BackgroundAppWindow(hwnd))
      break;

    DWORD styles, ex_styles;
    styles = (DWORD)GetWindowLongPtr(hwnd, GWL_STYLE);
    ex_styles = (DWORD)GetWindowLongPtr(hwnd, GWL_EXSTYLE);

	if (ex_styles & WS_EX_TOOLWINDOW)
		break;
      
    if (styles & WS_CHILD)
      break;

	//WinProg info;
    WindowEnumer::WINDOW_INFO info;

    RECT rc;
    ::GetWindowRect(hwnd, &rc);
	if (::IsRectEmpty(&rc))
		break;

    if (_tcscmp(class_name, L"TaskManagerWindow") == 0)
      break;

    if (_tcscmp(class_name, L"Program") == 0 && _tcscmp(window_name, L"Program Manager") == 0)
      break;

    DWORD process_id = 0;
    GetWindowThreadProcessId(hwnd, &process_id);
	if (0 == process_id) {
		break;
	}

    HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, process_id);
	if (!process) {
		break;
	}
      

    TCHAR module_name[MAX_PATH]{ 0 };
   // DWORD ret = GetModuleFileNameEx(process, NULL, module_name, MAX_PATH - 1);

	DWORD dwSize = MAX_PATH;
	DWORD ret = QueryFullProcessImageName(process, 0, module_name, &dwSize);
    CloseHandle(process);

	if (0 == ret)
		break;

	WindowEnumer::WINDOWS_ALL_INFO* win_infos = (WindowEnumer::WINDOWS_ALL_INFO*)data;

	info.x = rc.left;
	info.y = rc.top;
	info.width = rc.right - rc.left;
	info.height = rc.bottom - rc.top;
	info.sourceId = hwnd;
	info.sourceName = CT2A(window_name, CP_UTF8);
	info.moduleName = CT2A(module_name);
	info.isMinimizeWindow = ::IsIconic(hwnd);
	info.thumb.width = win_infos->fillWidth;
	info.thumb.height = win_infos->fillHeight;

	// 获取进程图标
	HDC window_dc = ::GetWindowDC(hwnd);
	int iconWidth, iconHeight;
	HBITMAP hBitmap = GetProcessIconBitmap(module_name, &iconWidth, &iconHeight);

	std::vector<BYTE> iconBuff;
	if (GetBitmapRGBAData(window_dc, hBitmap, iconBuff)) {
		StretchBitmap(window_dc, win_infos->iconWidth, win_infos->iconHeight, iconWidth, iconHeight,
			(const char*)&iconBuff[0], info.icon.data);

		info.icon.width = win_infos->iconWidth;
		info.icon.height = win_infos->iconHeight;
	}

	if (IsWindows8OrLater()) {
		//static SimpleWindow* simpleWnd = new SimpleWindow("background");
		static TaskWindow* simpleWnd = new TaskWindow();
		DrawThumbToWindow(*simpleWnd, hwnd, info.thumb.width, info.thumb.height);
		SendMessage(*simpleWnd, WM_KEYDOWN, VK_DELETE, (LPARAM)&info.thumb);
	}
	else {
		std::string pathName = info.moduleName;
		_strupr((char*)pathName.c_str());
		if (pathName.rfind("QQ.EXE") != std::string::npos) {
			MagnificationCapture magCapture;
			magCapture.LoadMagnificationLibrary();

			RECT rc;
			::GetWindowRect(hwnd, &rc);
			if(magCapture.CaptureFrame(hwnd, rc)){
				int width, height;
				std::vector<BYTE> imgData;
				magCapture.GetFrameInfo(width, height, imgData);

				uint8_t *bitmap_data = imgData.data();
				uint8_t* temp = new uint8_t[width * 4];
				for (int row = height >> 1; row >= 0; row--) {
					memcpy(temp, bitmap_data + width * 4 * row, width * 4);
					memcpy(bitmap_data + width * 4 * row, bitmap_data + width * 4 * (height - row - 1), width * 4);
					memcpy(bitmap_data + width * 4 * (height - row - 1), temp, width * 4);
				}
				delete[] temp;

				StretchBitmap(window_dc, win_infos->fillWidth, win_infos->fillHeight, width, height,
					(const char*)&imgData[0], info.thumb.data);
			}
		}
		else {
			// 获取窗口缩略图
			uint8_t* thumbdata = NULL;
			uint32_t width, height;
			if (GetWindowImageGDI(hwnd, &thumbdata, width, height)) {
				StretchBitmap(window_dc, win_infos->fillWidth, win_infos->fillHeight, width, height,
					(const char*)thumbdata, info.thumb.data);
				delete[] thumbdata;
			}
		}
	}
	
	ReleaseDC(hwnd, window_dc);

    auto windows = &win_infos->windows;
    auto itr = windows->find(info.moduleName);
    if (itr == std::end(*windows)) {
      windows->insert({ info.moduleName,{info} });
    }
    else {
      itr->second.push_back(info);
    }

  } while (0);

  return TRUE;
}

std::map<std::string, std::list<WindowEnumer::WINDOW_INFO>> WindowEnumer::EnumAllWindows(const std::list<std::string>& filters, int fillWidth, int fillHeight, int nIconSizeW, int nIconSizeH)
{
	WINDOWS_ALL_INFO win_infos;
	win_infos.fillWidth = fillWidth;
	win_infos.fillHeight = fillHeight;
	win_infos.iconWidth = nIconSizeW;
	win_infos.iconHeight = nIconSizeH;
	std::map<std::string, std::list<WindowEnumer::WINDOW_INFO>>& windows = win_infos.windows;
	::EnumWindows(WindowEnumCallback, (LPARAM)&win_infos);

	for (auto filter : filters) {
		auto itr = std::find_if(windows.begin(), windows.end(),
			[filter](const std::pair<std::string, std::list<WindowEnumer::WINDOW_INFO>>& a) {
			return (a.first == filter || a.first.find(filter) != std::wstring::npos);
		});
		if (itr != windows.end())
			windows.erase(itr);
	}

	return windows;
}

}
}