#include "stdafx.h"
#include "MagnificationCapture.h"

#include <iostream>
#include "SimpleWindow.h"

#define ROUND4(width) (((width-1)/4+1)*4)
#define APP_NAME _T("plug_in_screenshot_win")

static DWORD GetTlsIndex() {
	static const DWORD tls_index = TlsAlloc();
	return tls_index;
}

static BOOL isWindowIsAboveCaptureRegion(HWND hwndWindow, RECT rectCapture)
{
	RECT rectWindow;
	ZeroMemory(&rectWindow, sizeof(RECT));
	if (!GetWindowRect(hwndWindow, &rectWindow)) return FALSE;
	if (
		(
		(rectWindow.left >= rectCapture.left && rectWindow.left < rectCapture.right)
			||
			(rectWindow.right <= rectCapture.right && rectWindow.right > rectCapture.left)
			||
			(rectWindow.left <= rectCapture.left && rectWindow.right >= rectCapture.right)
			)
		&&
		(
		(rectWindow.top >= rectCapture.top && rectWindow.top < rectCapture.bottom)
			||
			(rectWindow.bottom <= rectCapture.bottom && rectWindow.bottom > rectCapture.top)
			||
			(rectWindow.top <= rectCapture.top && rectWindow.bottom >= rectCapture.bottom)
			)
		)
		return TRUE;
	else
		return FALSE;
}

static void CaptureMagnificationAPI_callback(HWND hwnd,
	void           *srcdata,
	MAGIMAGEHEADER  srcheader,
	void           *destdata,
	MAGIMAGEHEADER  destheader,
	RECT            unclipped,
	RECT            clipped,
	HRGN            dirty) {

	MagnificationCapture* magCapture = reinterpret_cast<MagnificationCapture*>(TlsGetValue(GetTlsIndex()));
	TlsSetValue(GetTlsIndex(), nullptr);
	if (magCapture) {
		magCapture->OnCaptured(srcdata, srcheader);
	}
}

MagnificationCapture::MagnificationCapture():
	magnificationLibrary(NULL),
	isCaptureSucceed(FALSE)
{
}

MagnificationCapture::~MagnificationCapture()
{
	UnLoadMagnificationLibrary();
}

void MagnificationCapture::UnLoadMagnificationLibrary()
{
	if (!magnificationLibrary) return;
	FreeLibrary(magnificationLibrary);

}

BOOL MagnificationCapture::LoadMagnificationLibrary()
{
	if (magnificationLibrary) return TRUE;

	magnificationLibrary = LoadLibrary(_T("Magnification"));
	if (!magnificationLibrary) return FALSE;

	MagInitialize = (MAGINITIALIZE)GetProcAddress(magnificationLibrary, "MagInitialize");
	if (!MagInitialize)
	{
		UnLoadMagnificationLibrary();
		return FALSE;
	}

	MagUninitialize = (MAGUNINITIALIZE)GetProcAddress(magnificationLibrary, "MagUninitialize");
	if (!MagUninitialize)
	{
		UnLoadMagnificationLibrary();
		return FALSE;
	}

	MagSetWindowSource = (MAGSETWINDOWSOURCE)GetProcAddress(magnificationLibrary, "MagSetWindowSource");
	if (!MagSetWindowSource)
	{
		UnLoadMagnificationLibrary();
		return FALSE;
	}

	MagSetWindowFilterList = (MAGSETWINDOWFILTERLIST)GetProcAddress(magnificationLibrary, "MagSetWindowFilterList");
	if (!MagSetWindowFilterList)
	{
		UnLoadMagnificationLibrary();
		return FALSE;
	}

	MagSetImageScalingCallback = (MAGSETIMAGESCALINGCALLBACK)GetProcAddress(magnificationLibrary, "MagSetImageScalingCallback");
	if (!MagSetImageScalingCallback)
	{
		UnLoadMagnificationLibrary();
		return FALSE;
	}

	return TRUE;
}

BOOL MagnificationCapture::CaptureFrame(HWND selectedHwnd, RECT rect)
{
	RECT round4Rect;
	HWND hwndHost;
	HWND hwndMag;
	const int MAX_EXCLUDEWIN_NUM = 64;
	HWND excludeWins[MAX_EXCLUDEWIN_NUM] = { 0 };
	int excludeWinsCount = 0;
	isCaptureSucceed = FALSE;

	if (!LoadMagnificationLibrary())
		return FALSE;

	if (!MagInitialize())
		return FALSE;

	HMODULE hInstance = nullptr;
	BOOL result =
		GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
			GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
			reinterpret_cast<char*>(&DefWindowProc), &hInstance);

	round4Rect = rect;
	round4Rect.right = round4Rect.left + ROUND4(round4Rect.right - round4Rect.left);

	WNDCLASSEXW wcex = {};
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.lpfnWndProc = &DefWindowProc;
	wcex.hInstance = hInstance;
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.lpszClassName = APP_NAME;

	// Ignore the error which may happen when the class is already registered.
	RegisterClassExW(&wcex);

	/* Create the host window that will store the mag child window */
	hwndHost = CreateWindowEx(0x08000000 | 0x080000 | 0x80 | 0x20, APP_NAME, NULL, 0x80000000,
		0, 0, 0, 0, NULL, NULL, hInstance, NULL);

	if (!hwndHost) {
		MagUninitialize();
		return FALSE;
	}

	SetLayeredWindowAttributes(hwndHost, (COLORREF)0, (BYTE)255, (DWORD)0x02);
	/* Create the mag child window inside the host window */
	hwndMag = CreateWindow(WC_MAGNIFIER, TEXT("MagnifierWindow"),
		WS_CHILD /*| MS_SHOWMAGNIFIEDCURSOR*/  /*| WS_VISIBLE*/,
		0, 0, round4Rect.right - round4Rect.left, round4Rect.bottom - round4Rect.top,
		hwndHost, NULL, hInstance, NULL);

	if (!MagSetImageScalingCallback(hwndMag, (MagImageScalingCallback)CaptureMagnificationAPI_callback)) {
		DestroyWindow(hwndHost);
		MagUninitialize();
		return FALSE;
	}

	for (HWND nextWindow = GetNextWindow(selectedHwnd, GW_HWNDPREV); nextWindow != NULL;
		nextWindow = GetNextWindow(nextWindow, GW_HWNDPREV)) {
		if (isWindowIsAboveCaptureRegion(nextWindow, rect)) {
			/* This api can't work with more than 24 windows. we stop on the 24 window */
			excludeWins[excludeWinsCount++] = nextWindow;
			if (excludeWinsCount >= MAX_EXCLUDEWIN_NUM)
				break;
		}
		
	}

	TlsSetValue(GetTlsIndex(), this);

	if (excludeWinsCount) {
		BOOL bFilter = MagSetWindowFilterList(hwndMag, MW_FILTERMODE_EXCLUDE, excludeWinsCount, excludeWins);
		if (!bFilter) {
			DestroyWindow(hwndHost);
			MagUninitialize();
			return FALSE;
		}
	}

	if (!MagSetWindowSource(hwndMag, round4Rect) || !isCaptureSucceed) {
		DestroyWindow(hwndHost);
		MagUninitialize();
		return FALSE;
	}

	DestroyWindow(hwndHost);
	MagUninitialize();
	UnregisterClass(APP_NAME, GetModuleHandle(NULL));

	return TRUE;
}

void MagnificationCapture::OnCaptured(void * data, const MAGIMAGEHEADER & header)
{
	isCaptureSucceed = TRUE;

	int captured_bytes_per_pixel = header.cbSize / header.width / header.height;
	if (header.format != GUID_WICPixelFormat32bppRGBA) {
		std::cout
			<< "Output format does not match the captured format: "
			<< "width = " << header.width << ", "
			<< "height = " << header.height << ", "
			<< "stride = " << header.stride << ", "
			<< "bpp = " << captured_bytes_per_pixel << ", "
			<< "pixel format RGBA ? "
			<< (header.format == GUID_WICPixelFormat32bppRGBA) << ".";
		return;
	}

	img_header = header;
	cache_img.resize(header.cbSize);
	memcpy(&cache_img[0], data, header.cbSize);

	static SimpleWindow* pImageWnd = new SimpleWindow("ScaleImage");

	HWND hWnd = pImageWnd->GetView();
	HBITMAP hBitmap = ::CreateBitmap(header.width, header.height, 1, 32, data);
	HDC hWndDC = ::GetDC(hWnd);
	HDC hMemDc = ::CreateCompatibleDC(hWndDC);
	HBITMAP hOldBitmap = (HBITMAP)::SelectObject(hMemDc, hBitmap);
	//::BitBlt(hWndDC, 0, 0, header.width, header.height, hMemDc, 0, 0, SRCCOPY);
	SetStretchBltMode(hWndDC, HALFTONE);
	::StretchBlt(hWndDC, 0, 0, 1280, 720, hMemDc, 0, 0, header.width, header.height, SRCCOPY);

	::SelectObject(hMemDc, hOldBitmap);
	::DeleteObject(hBitmap);
	::DeleteDC(hMemDc);
	::ReleaseDC(hWnd, hWndDC);

	Sleep(2000);
}

BOOL MagnificationCapture::GetFrameInfo(int & imgWidth, int & imgHeight, std::vector<BYTE>& imgData)
{
	if (isCaptureSucceed == FALSE)
		return FALSE;

	imgWidth = img_header.width;
	imgHeight = img_header.height;
	imgData = cache_img;

	return TRUE;
}
