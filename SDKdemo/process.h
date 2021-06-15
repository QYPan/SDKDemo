#pragma once

#include <Windows.h>
#include <string>
#include <vector>

#ifdef UNICODE
#define astring std::wstring
#else
#define astring std::string
#endif

namespace app {
namespace utils {

/*
* Get icon of specified process by path and convert HICCON to HBITMAP
*/
HBITMAP GetProcessIconBitmap(const astring& path, int* width, int* height);

/*
* Open and select specified item by path
* @param path
*/
bool OpenAndSelectItem(const astring& path);

/*
* Open url with default browser
*/
void GotoUrl(const astring& url);

/*
* Send copy data to specified window by class name or window name with msg WM_COPYDATA
* @param class_name
* @param window_name
* @param data
* @param timeouts default INFINITE
*/
bool SendCopyDataToWindow(const astring& class_name, const astring& window_name, const COPYDATASTRUCT& data, int timeouts = INFINITE);

/*
* Send copy data to specified window by HWND with msg WM_COPYDATA
* @param hwnd
* @param data
* @param timeouts default INFINITE
*/
bool SendCopyDataToWindow(HWND hwnd, const COPYDATASTRUCT& data, int timeouts = INFINITE);

/**
* Terminat process with specified file name
*/
void TerminateProcess(const astring& file_name, bool once = false);

/*
* Copy string to clicpboard
*/
bool CopyToClipBoard(const astring& data);

bool GetBitmapRGBAData(HDC hDC, HBITMAP hBitmap, std::vector<BYTE>& imagedata);

bool DrawThumbToWindow(HWND hDestWnd, HWND hTargetWnd, int maxWidth, int maxHeight);

bool GetDesktopAREAData(RECT& rcArea, std::vector<BYTE>& imagedata, int maxWidth, int maxHeight);

void SaveToDisk(const TCHAR* filename, HBITMAP hBitmap, std::vector<BYTE>& imagedata);

bool StretchBitmap(HDC hDC, int dstW, int dstH, int srcW, int srcH, const char* srcdata, std::vector<BYTE>& outdata);

}
}