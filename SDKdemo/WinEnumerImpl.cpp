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

  auto monitors = ((std::list<WindowEnumer::MONITOR_INFO>*)data);

  WindowEnumer::MONITOR_INFO info;
  info.index = monitors->size();
  info.rc = info_ex.rcMonitor;
  info.name = info_ex.szDevice;
  info.is_primary = info_ex.dwFlags & MONITORINFOF_PRIMARY;


  monitors->emplace_back(info);

  return true;
}

std::list<WindowEnumer::MONITOR_INFO> WindowEnumer::EnumAllMonitors()
{
  std::list<WindowEnumer::MONITOR_INFO> monitors;

  ::EnumDisplayMonitors(NULL, NULL, MonitorEnumCallback, (LPARAM)&monitors);

  return monitors;
}

WindowEnumer::MONITOR_INFO WindowEnumer::GetMonitorInfoByIndex(int index)
{
  auto monitors = EnumAllMonitors();
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

    TCHAR class_name[MAX_PATH]{ 0 };
    ::GetClassName(hwnd, class_name, MAX_PATH);

    TCHAR window_name[MAX_PATH]{ 0 };
    ::GetWindowText(hwnd, window_name, MAX_PATH);

    if (_tcscmp(class_name, L"TaskManagerWindow") == 0)
      break;

    if (_tcscmp(class_name, L"Program") == 0 && _tcscmp(window_name, L"Program Manager") == 0)
      break;

    DWORD process_id = 0;
    GetWindowThreadProcessId(hwnd, &process_id);
    if (0 == process_id)
      break;

    HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, process_id);
    if (!process)
      break;

    TCHAR module_name[MAX_PATH]{ 0 };
    DWORD ret = GetModuleFileNameEx(process, NULL, module_name, MAX_PATH - 1);

    CloseHandle(process);

    if (0 == ret)
      break;


	info.x = rc.left;
	info.y = rc.top;
	info.width = rc.right - rc.left;
	info.height = rc.bottom - rc.top;
	info.sourceId = hwnd;
	info.sourceName = CT2A(window_name, CP_UTF8);
	info.moduleName = CT2A(module_name);
	info.isMinimizeWindow = ::IsIconic(hwnd);

	int thumbWidth = 0, thumbHeight = 0;
	info.hIcon = GetProcessIconBitmap(module_name, &thumbWidth, &thumbHeight);
    
    auto windows = (std::map<std::string, std::list<WindowEnumer::WINDOW_INFO>>*)data;
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

std::map<std::string, std::list<WindowEnumer::WINDOW_INFO>> WindowEnumer::EnumAllWindows(const std::list<std::string>& filters)
{
	std::map<std::string, std::list<WindowEnumer::WINDOW_INFO>> windows;
	::EnumWindows(WindowEnumCallback, (LPARAM)&windows);

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