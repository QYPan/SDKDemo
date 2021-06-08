#include "stdafx.h"
#include "process.h"

#include <Windows.h>
#include <ShellAPI.h>
#include <ShlObj.h>
#include <ShObjIdl.h>
#include <tchar.h>
#include <GdiPlus.h>
#include <TlHelp32.h>
#include <MMSystem.h>
#include <dwmapi.h>

#pragma comment(lib, "Dwmapi.lib")

namespace app {
namespace utils {

int GetEncoderClsid(const TCHAR* format, CLSID* pClsid)
{
  UINT num = 0, size = 0;  Gdiplus::GetImageEncodersSize(&num, &size);  if (size == 0)    return -1;  // Failure  

  Gdiplus::ImageCodecInfo* pImageCodecInfo = (Gdiplus::ImageCodecInfo*)(malloc(size));
  Gdiplus::GetImageEncoders(num, size, pImageCodecInfo);

  bool found = false;
  for (UINT ix = 0; !found && ix < num; ++ix)
  {
    if (_wcsicmp(pImageCodecInfo[ix].MimeType, format) == 0)
    {
      *pClsid = pImageCodecInfo[ix].Clsid;
      found = true;
      break;
    }
  }

  free(pImageCodecInfo);

  return found;
}

HBITMAP ConvertHicon2HBitmap(HICON hIcon, int* width, int* height)
{
  HBITMAP hBitmap = nullptr;

  if (hIcon == NULL) return nullptr;

  ICONINFO icInfo = { 0 };
  if (!::GetIconInfo(hIcon, &icInfo))
    return nullptr;

  BITMAP bitmap;
  GetObject(icInfo.hbmColor, sizeof(BITMAP), &bitmap);

  *width = bitmap.bmWidth;
  *height = bitmap.bmHeight;

  Gdiplus::Bitmap* pBitmap = NULL;
  Gdiplus::Bitmap* pWrapBitmap = NULL;

  do {
    if (bitmap.bmBitsPixel != 32)
    {
      pBitmap = Gdiplus::Bitmap::FromHICON(hIcon);
    }
    else {
      pWrapBitmap = Gdiplus::Bitmap::FromHBITMAP(icInfo.hbmColor, NULL);
	  if (!pWrapBitmap) {
		  DWORD dwError = GetLastError();
		  break;
	  }
        
      Gdiplus::BitmapData bitmapData;
      Gdiplus::Rect rcImage(0, 0, pWrapBitmap->GetWidth(), pWrapBitmap->GetHeight());
      pWrapBitmap->LockBits(&rcImage, Gdiplus::ImageLockModeRead, pWrapBitmap->GetPixelFormat(), &bitmapData);
      pBitmap = new (Gdiplus::Bitmap)(bitmapData.Width, bitmapData.Height, bitmapData.Stride, PixelFormat32bppARGB, (BYTE*)bitmapData.Scan0);
      pWrapBitmap->UnlockBits(&bitmapData);
    }


    pBitmap->GetHBITMAP(Gdiplus::Color::Black, &hBitmap);

    /*CLSID encoderCLSID;
    GetEncoderClsid(_T("p_w_picpath/png"), &encoderCLSID);
    Gdiplus::Status st = pBitmap->Save(lpszPicFileName, &encoderCLSID, NULL);
    if (st != Gdiplus::Ok)
        break;*/

  } while (0);

  delete pBitmap;

  if (pWrapBitmap)
    delete pWrapBitmap;

  DeleteObject(icInfo.hbmColor);
  DeleteObject(icInfo.hbmMask);

  return hBitmap;
}

HBITMAP GetProcessIconBitmap(const astring& path, int* width, int* height)
{
  HICON icon = nullptr;
  HBITMAP bitmap = nullptr;

  do {
    ExtractIconEx(path.c_str(), 0, &icon, NULL, 1);
    if (!icon) break;

    bitmap = ConvertHicon2HBitmap(icon, width, height);
  } while (0);

  if (icon) DestroyIcon(icon);

  return bitmap;
}

bool OpenAndSelectItem(const astring& path)
{
  LPSHELLFOLDER desktop;
  ::CoInitialize(NULL);

  if (FAILED(SHGetDesktopFolder(&desktop)))
  {
    ::CoUninitialize();
    return false;
  }

  LPITEMIDLIST lpidl = nullptr;
  ULONG eaten;

  TCHAR pstr_path[MAX_PATH + 1] = { 0 };
  _tcscpy_s(pstr_path, MAX_PATH + 1, path.c_str());

  HRESULT hr = desktop->ParseDisplayName(NULL, 0, pstr_path, &eaten, &lpidl, NULL);
  if (FAILED(hr))
  {
    desktop->Release();
    ::CoUninitialize();
    return false;
  }

  LPCITEMIDLIST cpidl = lpidl;
  bool success = false;
  HMODULE hShell32DLL = ::LoadLibrary(TEXT("shell32.dll"));

  if (hShell32DLL)
  {
    typedef HRESULT(WINAPI* pSelFun)(LPCITEMIDLIST pidlFolder, UINT cidl, LPCITEMIDLIST* apidl, DWORD dwFlags);

    pSelFun sh_func = (pSelFun)::GetProcAddress(hShell32DLL, "SHOpenFolderAndSelectItems");

    ::FreeLibrary(hShell32DLL);

    if (sh_func != NULL)
    {
      success = SUCCEEDED(sh_func(cpidl, 0, NULL, 0));
    }
  }

  desktop->Release();

  ::CoUninitialize();

  return success;
}

void GotoUrl(const astring& url)
{
  ShellExecute(NULL, TEXT("open"), url.c_str(), NULL, NULL, SW_HIDE);
}

bool SendCopyDataToWindow(const astring& class_name, const astring& window_name, const COPYDATASTRUCT& data, int timeouts)
{
  HWND hwnd = FindWindow(class_name.c_str(), window_name.c_str());

  return SendCopyDataToWindow(hwnd, data, timeouts);
}

bool SendCopyDataToWindow(HWND hwnd, const COPYDATASTRUCT& data, int timeouts)
{
  DWORD res = 0;

  return (::SendMessageTimeout(hwnd, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&data, SMTO_BLOCK, timeouts, &res) != 0);
}

void TerminateProcess(const astring& file_name, bool once)
{
  HANDLE         snapshot = NULL;
  PROCESSENTRY32 pe32;

  snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (snapshot == INVALID_HANDLE_VALUE)
    return;

  pe32.dwSize = sizeof(PROCESSENTRY32);

  if (Process32First(snapshot, &pe32))
  {
    do
    {
      if (file_name == pe32.szExeFile)
      {
        HANDLE hprocess = OpenProcess(PROCESS_ALL_ACCESS, true, pe32.th32ProcessID);
        ::TerminateProcess(hprocess, 0);

        if (once) {
          CloseHandle(snapshot);
          return;
        }
      }
    } while (Process32Next(snapshot, &pe32));
  }

  CloseHandle(snapshot);
}

bool CopyToClipBoard(const astring& data)
{
  bool ret = false;
  bool opended = false;

  HGLOBAL memory = nullptr;

  do {
    if (!OpenClipboard(NULL))
      break;

    opended = true;

    EmptyClipboard();

    if ((memory = GlobalAlloc(GMEM_MOVEABLE, (data.length() + 1) * sizeof(TCHAR))) == nullptr)
      break;

    LPTSTR lpDest = (LPTSTR)GlobalLock(memory);
    if (!lpDest)
      break;

    _tcscpy_s(lpDest, data.length() + 1, data.c_str());

    GlobalUnlock(memory);

    UINT format = (sizeof(TCHAR) == sizeof(WCHAR)) ? CF_UNICODETEXT : CF_TEXT;
    HANDLE clipboard = SetClipboardData(format, memory);

    if (clipboard == NULL && GetLastError() != S_OK)
      break;

    ret = true;
  } while (0);

  if (opended)
    CloseClipboard();

  return ret;
}

bool GetBitmapRGBAData(HDC hDC, HBITMAP hBitmap, std::vector<BYTE>& imagedata)
{
	struct { BITMAPINFO info; RGBQUAD moreColors[255]; } fbi = { 0 };
	BITMAPINFOHEADER &bi = fbi.info.bmiHeader;
	bi.biSize = sizeof(BITMAPINFOHEADER);

	GetDIBits(hDC, hBitmap, 0, 0, NULL, &fbi.info, DIB_RGB_COLORS);
	if (!bi.biSizeImage)
		return false;

	imagedata.resize(bi.biSizeImage);
	int ret = GetDIBits(hDC, hBitmap, 0, fbi.info.bmiHeader.biHeight, &imagedata[0], (BITMAPINFO*)&fbi.info, DIB_RGB_COLORS);

	return ret != 0 ? true : false;
}

bool DrawThumbToWindow(HWND hDestWnd, HWND hTargetWnd, int maxWidth, int maxHeight)
{
	if (!::IsWindow(hDestWnd))
		return false;

	HTHUMBNAIL thumbNail;
	if (FAILED(::DwmRegisterThumbnail(hDestWnd, hTargetWnd, &thumbNail)))
		return false;

	if (!thumbNail)
		return false;

	SIZE size{ 0 };
	::DwmQueryThumbnailSourceSize(thumbNail, &size);

	int sw = size.cx;
	int sh = size.cy;
	
	int destWidth = sw;
	int destHeight = sh;
	if (destWidth > maxWidth) {
		destWidth = maxWidth;
		destHeight = 1.0 * destWidth * sh / sw;
	}

	if (destHeight > maxHeight) {
		destHeight = maxHeight;
		destWidth = 1.0 * destHeight * sw / sh;
	}

	RECT rcDest = { 0 };
	rcDest.right = destWidth;
	rcDest.bottom = destHeight;

	DWM_THUMBNAIL_PROPERTIES prop = { 0 };
	prop.dwFlags = DWM_TNP_RECTDESTINATION | DWM_TNP_RECTSOURCE | DWM_TNP_VISIBLE;
	prop.fVisible = TRUE;
	prop.rcDestination = rcDest;
	prop.rcSource = { 0, 0, sw, sh };

	::SetWindowPos(hDestWnd, NULL, 0, 0, destWidth, destHeight, SWP_NOMOVE | SWP_NOACTIVATE);
	HRESULT hr = ::DwmUpdateThumbnailProperties(thumbNail, &prop);
	if (FAILED(hr))
		return false;

	int imgWidth, imgHeight;
	std::vector<BYTE> vData;
	GetPictureFromHWND(hDestWnd, imgWidth, imgHeight, vData);

	return true;
}

bool  GetPictureFromHWND(HWND hWnd, int& nWidth, int& nHeight, std::vector<BYTE>& imagedata)
{
	RECT rcTarget;
	::GetClientRect(hWnd, &rcTarget);
	nWidth = rcTarget.right - rcTarget.left;
	nHeight = rcTarget.bottom - rcTarget.top;

	HDC hDC = ::GetDC(hWnd);
	HDC hCompatibleDC = ::CreateCompatibleDC(hDC);
	HBITMAP hCompatibleBitmap = ::CreateCompatibleBitmap(hDC, nWidth, nHeight);

	HBITMAP hOldBitmap = (HBITMAP)SelectObject(hCompatibleDC, hCompatibleBitmap);
	::BitBlt(hCompatibleDC, 0, 0, nWidth, nHeight, hDC, 0, 0, SRCCOPY);

	bool ret = GetBitmapRGBAData(hCompatibleDC, hCompatibleBitmap, imagedata);
	
	SelectObject(hCompatibleDC, hOldBitmap);
	::DeleteObject(hCompatibleBitmap);
	::DeleteObject(hCompatibleDC);
	::ReleaseDC(hWnd, hDC);

	return ret;
}

bool GetDesktopAREAData(RECT & rcArea, int & outWidth, int & outHeight, std::vector<BYTE>& imagedata)
{
	outWidth = rcArea.right - rcArea.left;
	outHeight = rcArea.bottom - rcArea.top;

	HDC hDC = ::GetDC(NULL);
	HDC hCompatibleDC = ::CreateCompatibleDC(hDC);
	HBITMAP hCompatibleBitmap = ::CreateCompatibleBitmap(hDC, outWidth, outHeight);
	HBITMAP hOldBitmap = (HBITMAP)SelectObject(hCompatibleDC, hCompatibleBitmap);
	::StretchBlt(hCompatibleDC, 0, 0, outWidth, outHeight, hDC, rcArea.left, rcArea.top, outWidth, outHeight, SRCCOPY);

	bool ret = GetBitmapRGBAData(hCompatibleDC, hCompatibleBitmap, imagedata);

	/*TCHAR* filename = _T("test.bmp");
	SaveToDisk(filename, hCompatibleBitmap, imagedata);*/

	SelectObject(hCompatibleDC, hOldBitmap);
	::DeleteObject(hCompatibleBitmap);
	::DeleteObject(hCompatibleDC);
	::ReleaseDC(NULL, hDC);
	
	return ret;
}

void SaveToDisk(const TCHAR * filename, HBITMAP hBitmap, std::vector<BYTE>& imagedata)
{
	BITMAP bm;
	GetObject(hBitmap, sizeof(bm), &bm);

	BITMAPINFOHEADER bi = { 0 };
	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = bm.bmWidth;
	bi.biHeight = bm.bmHeight;
	bi.biPlanes = bm.bmPlanes;
	bi.biBitCount = bm.bmBitsPixel;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = bm.bmHeight * bm.bmWidthBytes;

	BITMAPFILEHEADER bfh = { 0 };
	bfh.bfType = ((WORD)('M' << 8) | 'B');
	bfh.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + bi.biSizeImage;
	bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	HANDLE hFile = CreateFile(filename, GENERIC_WRITE, 0, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

	DWORD dwWrite;
	WriteFile(hFile, &bfh, sizeof(BITMAPFILEHEADER), &dwWrite, NULL);
	WriteFile(hFile, &bi, sizeof(BITMAPINFOHEADER), &dwWrite, NULL);
	WriteFile(hFile, &imagedata[0], bi.biSizeImage, &dwWrite, NULL);
	CloseHandle(hFile);
}

}
}


