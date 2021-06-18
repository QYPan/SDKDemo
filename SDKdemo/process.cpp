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

#pragma warning(disable:4996)
#pragma comment(lib, "Dwmapi.lib")

//#include "SimpleWindow.h"

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

	::SetWindowPos(hDestWnd, NULL, -3000, -3000, destWidth, destHeight, SWP_NOACTIVATE);
	HRESULT hr = ::DwmUpdateThumbnailProperties(thumbNail, &prop);
	if (FAILED(hr))
		return false;

	return true;
}

bool GetDesktopAREAData(RECT & rcArea, std::vector<BYTE>& imagedata, int fillWidth, int fillHeight)
{
	int realWidth = rcArea.right - rcArea.left;
	int realHeight = rcArea.bottom - rcArea.top;

	HDC hDC = ::GetDC(NULL);
	HDC hCompatibleDC = ::CreateCompatibleDC(hDC);
	HBITMAP hCompatibleBitmap = ::CreateCompatibleBitmap(hDC, fillWidth, fillHeight);
	HBITMAP hOldBitmap = (HBITMAP)SelectObject(hCompatibleDC, hCompatibleBitmap);

	SetStretchBltMode(hCompatibleDC, HALFTONE);
	::StretchBlt(hCompatibleDC, 0, 0, fillWidth, fillHeight, hDC, rcArea.left, rcArea.top, realWidth, realHeight, SRCCOPY);

	bool ret = GetBitmapRGBAData(hCompatibleDC, hCompatibleBitmap, imagedata);

	int width = fillWidth;
	int height = fillHeight;
	uint8_t* bitmap_data = imagedata.data();
	uint8_t* temp = new uint8_t[width * 4];
	for (int row = height >> 1; row >= 0; row--) {
		memcpy(temp, bitmap_data + width * 4 * row, width * 4);
		memcpy(bitmap_data + width * 4 * row, bitmap_data + width * 4 * (height - row - 1), width * 4);
		memcpy(bitmap_data + width * 4 * (height - row - 1), temp, width * 4);
	}
	delete[] temp;

	int imageSize = width * 4 * height;
	for (int i = 0; i < imageSize; i += 4) {
		bitmap_data[i + 3] = 0xFF;
	}

	/*TCHAR* filename = _T("test.bmp");
	SaveToDisk(filename, hCompatibleBitmap, imagedata);*/


	/*static SimpleWindow* pImageWnd1 = new SimpleWindow("ScaleImage112");
	::BitBlt(::GetDC(pImageWnd1->GetView()), 0, 0, fillWidth, fillHeight, hCompatibleDC, 0, 0, SRCCOPY);
	Sleep(3000);*/

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

bool StretchBitmap(HDC hDC, int dstW, int dstH, int srcW, int srcH, const char * srcdata, std::vector<BYTE>& outdata)
{
	BITMAPINFO bmi = {};
	bmi.bmiHeader.biHeight = -srcH;
	bmi.bmiHeader.biWidth = srcW;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
	bmi.bmiHeader.biSizeImage = srcW * 4 * srcH;

	bool bSucc = false;
	HDC mem_dc = CreateCompatibleDC(hDC);
	HBITMAP mem_bitmap = CreateCompatibleBitmap(hDC, dstW, dstH);
	SelectObject(mem_dc, mem_bitmap);

	do {
		// 缩放图片
		SetStretchBltMode(mem_dc, HALFTONE);
		int ret = StretchDIBits(mem_dc, 0, 0, dstW, dstH, 0, 0, srcW, srcH,
			srcdata, &bmi, DIB_RGB_COLORS, SRCCOPY);
		if (!ret)
			break;

		// 获取图片数据
		outdata.resize(dstW * 4 * dstH);
		if (!GetBitmapRGBAData(mem_dc, mem_bitmap, outdata))
			break;

		int imageSize = dstW * 4 * dstH;
		BYTE* bitmap_data = outdata.data();
		for (int i = 0; i < imageSize; i += 4) {
			bitmap_data[i + 3] = 0xFF;
		}

		bSucc = true;
	} while (false);
	
	/*static SimpleWindow* pImageWnd1 = new SimpleWindow("ScaleImage112");
	HDC hWndDC = ::GetDC(pImageWnd1->GetView());
	::BitBlt(hWndDC, 0, 0, dstW, dstH, mem_dc, 0, 0, SRCCOPY);
	ReleaseDC(pImageWnd1->GetView(), hWndDC);
	Sleep(2000);*/

	DeleteObject(mem_bitmap);
	DeleteObject(mem_dc);

	return bSucc;
}

static bool GetOsVersion(int* major, int* minor, int* build) {
	OSVERSIONINFO info = { 0 };
	info.dwOSVersionInfoSize = sizeof(info);
	if (GetVersionEx(&info)) {
		if (major) *major = info.dwMajorVersion;
		if (minor) *minor = info.dwMinorVersion;
		if (build) *build = info.dwBuildNumber;
		return true;
	}
	return false;
}

enum WindowsMajorVersions {
	kWindows2000 = 5,
	kWindowsVista = 6,
	kWindows10 = 10,
};

bool IsWindows8OrLater() {
	int major = 0, minor = 0;
	typedef void(__stdcall * NTPROC)(DWORD*, DWORD*, DWORD*);
	HINSTANCE hinst = LoadLibrary(L"ntdll.dll");
	if (hinst) {
		NTPROC proc = (NTPROC)GetProcAddress(hinst, "RtlGetNtVersionNumbers");
		if (proc) {
			DWORD dwMajor, dwMinor, dwBuildNumber;
			proc(&dwMajor, &dwMinor, &dwBuildNumber);
			dwBuildNumber &= 0xffff;
			if ((dwMajor > kWindowsVista) || (dwMajor == kWindowsVista && dwMinor > 1)) {
				FreeLibrary(hinst);
				return true;
			}
			else {
				FreeLibrary(hinst);
				return false;
			}
		}
		else {
			FreeLibrary(hinst);
			return (GetOsVersion(&major, &minor, nullptr) &&
				((major > kWindowsVista) || (major == kWindowsVista && minor > 1)));
		}
	}
	else {
		return (GetOsVersion(&major, &minor, nullptr) &&
			((major > kWindowsVista) || (major == kWindowsVista && minor > 1)));
	}
}

bool GetWindowImageGDI(HWND window, uint8_t** data, uint32_t &width, uint32_t &height) {
	RECT rect;
	if (!GetWindowRect(window, &rect)) {
		return false;
	}
	HDC window_dc = GetWindowDC(window);
	if (!window_dc) {
		return false;
	}

	width = rect.right - rect.left;
	height = rect.bottom - rect.top;

	int bytes_per_row = width * 4;

	// Describe a device independent bitmap (DIB) that is the size of the desktop.
	BITMAPINFO bmi = {};
	bmi.bmiHeader.biHeight = -(rect.bottom - rect.top);
	bmi.bmiHeader.biWidth = width;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
	bmi.bmiHeader.biSizeImage = bytes_per_row * height;

	HANDLE section_handle = nullptr;
	uint8_t *bitmap_data = nullptr;
	HBITMAP bitmap =
		CreateDIBSection(window_dc, &bmi, DIB_RGB_COLORS, (void **)&bitmap_data, section_handle, 0);
	if (!bitmap) {
		return false;
	}

	HDC mem_dc = CreateCompatibleDC(window_dc);
	HGDIOBJ previous_object = SelectObject(mem_dc, bitmap);
	if (!previous_object || previous_object == HGDI_ERROR) {
		return false;
	}
	bool result;
	if (IsWindows8OrLater()) {
		result = PrintWindow(window, mem_dc, 2);
	}
	else {
		result = PrintWindow(window, mem_dc, 0);
	}
	if (!result) {
		result = (BitBlt(mem_dc, 0, 0, width, height, window_dc, 0, 0,
			SRCCOPY | CAPTUREBLT) != FALSE);
	}
	if (!result) {
		return false;
	}

	uint8_t* temp = new uint8_t[width * 4];
	for (int row = height >> 1; row >= 0; row--) {
		memcpy(temp, bitmap_data + width * 4 * row, width * 4);
		memcpy(bitmap_data + width * 4 * row, bitmap_data + width * 4 * (height - row - 1), width * 4);
		memcpy(bitmap_data + width * 4 * (height - row - 1), temp, width * 4);
	}
	delete[] temp;

	*data = new uint8_t[bmi.bmiHeader.biSizeImage];
	memcpy(*data, bitmap_data, bmi.bmiHeader.biSizeImage);
	if (bitmap) {
		DeleteObject(bitmap);
	}
	// Select back the previously selected object to that the device contect
	// could be destroyed independently of the bitmap if needed.
	SelectObject(mem_dc, NULL);
	DeleteDC(mem_dc);
	ReleaseDC(window, window_dc);
	return true;
}

void SaveToDisk1(const TCHAR * filename, int width, int height, int widthBytes, BYTE* imagedata)
{

	BITMAPINFOHEADER bi = { 0 };
	bi.biSize = sizeof(bi);;
	bi.biWidth = width;
	bi.biHeight = -height;
	bi.biPlanes = 1;
	bi.biBitCount = 32;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = height * widthBytes;

	BITMAPFILEHEADER bfh = { 0 };
	bfh.bfType = ((WORD)('M' << 8) | 'B');
	bfh.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + bi.biSizeImage;
	bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	HANDLE hFile = CreateFile(filename, GENERIC_WRITE, 0, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

	DWORD dwWrite;
	WriteFile(hFile, &bfh, sizeof(BITMAPFILEHEADER), &dwWrite, NULL);
	WriteFile(hFile, &bi, sizeof(BITMAPINFOHEADER), &dwWrite, NULL);
	WriteFile(hFile, imagedata, bi.biSizeImage, &dwWrite, NULL);
	CloseHandle(hFile);
}

}
}


