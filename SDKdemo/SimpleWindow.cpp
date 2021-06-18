#include "stdafx.h"
#include "SimpleWindow.h"
#include "process.h"
#include "WinEnumerImpl.h"

const char kSimpleWindowClass[] = "AEC858EC-E51C-4211-A464-CE538AA27AA6";

SimpleWindow::SimpleWindow(std::string window_title) : hwnd_(nullptr), stop_(false) {
  static ATOM wc_atom = 0;
  if (wc_atom == 0) {
    WNDCLASSA wc = {};

    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW);
    wc.lpszClassName = kSimpleWindowClass;

    wc_atom = RegisterClassA(&wc);
    if (wc_atom == 0) return;
  }

  HANDLE created = CreateEvent(NULL, FALSE, FALSE, NULL);

  looper_ = std::unique_ptr<std::thread>(new std::thread([this, window_title, &created] {
    hwnd_ = CreateWindowA(kSimpleWindowClass, window_title.c_str(), WS_POPUP, 0, 0,
                          static_cast<int>(1280), static_cast<int>(720), NULL, NULL, NULL, NULL);

    SetEvent(created);

    if (!hwnd_) {
      return;
    }
    // looper has to has the same thread as CreateWindow

    MSG msg;
    BOOL ret;
    while ((ret = GetMessage(&msg, NULL, 0, 0)) != 0) {
      if (stop_) {
        break;
      }
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
    CloseWindow(hwnd_);
    DestroyWindow(hwnd_);
  }));

  DWORD dwResult = WaitForSingleObject(created, -1);
  if (dwResult != WAIT_OBJECT_0) {
    stop_ = true;
    return;
  }

  if (hwnd_ == NULL) {
    stop_ = true;
    return;
  }

  CloseHandle(created);

  ShowWindow(hwnd_, SW_SHOWNOACTIVATE);
}

SimpleWindow::~SimpleWindow() {
  if (hwnd_ != NULL) {
    stop_ = true;
    PostMessage(hwnd_, WM_PAINT, 0, 0);
    if (looper_ && looper_->joinable()) {
      looper_->join();
      looper_.reset();
    }
    hwnd_ = NULL;
  }
}

HWND SimpleWindow::GetView() { return hwnd_; }

LRESULT WINAPI SimpleWindow::WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
  if (msg == WM_DESTROY || (msg == WM_CHAR && wparam == VK_RETURN)) {
    PostQuitMessage(0);
    return 0;
  }
  else if (msg == WM_KEYDOWN && wparam == VK_DELETE) {
	  app::utils::WindowEnumer::IMAGE_INFO* info = (app::utils::WindowEnumer::IMAGE_INFO*)lparam;
	
	  HDC window_dc = GetDC(hwnd);
	  uint8_t* thumbdata = NULL;
	  uint32_t width, height;
	  if (app::utils::GetWindowImageGDI(hwnd, &thumbdata, width, height)) {
		  app::utils::StretchBitmap(window_dc, info->width, info->height, width, height,
			  (const char*)thumbdata, info->data);
		  delete[] thumbdata;
	  }

	 // app::utils::SaveToDisk1(L"test22.bmp", width, height, width*4, outData.data());
	  ReleaseDC(hwnd, window_dc);
  }

  return DefWindowProcA(hwnd, msg, wparam, lparam);
}
