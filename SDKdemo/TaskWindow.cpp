#include "stdafx.h"
#include "TaskWindow.h"
#include "process.h"
#include "WinEnumerImpl.h"

#include <objbase.h>

using namespace std;

static string CreateGUID()
{
	GUID guid = { 0 };
	if (S_OK != ::CoCreateGuid(&guid))
	{
		return string("CED858EC-EV1C-4211-A464-CE538AA27AA6");
	}

	char szBuffer[64] = { 0 };
	_snprintf_s(
		szBuffer,sizeof(szBuffer),sizeof(szBuffer),
		"{%08X-%04X-%04x-%02X%02X-%02X%02X%02X%02X%02X%02X}",
		guid.Data1,guid.Data2,guid.Data3,
		guid.Data4[0],guid.Data4[1],guid.Data4[2],guid.Data4[3],
		guid.Data4[4],guid.Data4[5],guid.Data4[6],guid.Data4[7]);
	return string(szBuffer);
}

TaskWindow::TaskWindow():
	m_hWnd(NULL)
{
	initialize();
}

TaskWindow::~TaskWindow()
{
	unInit();
}

HWND TaskWindow::GetView()
{
	return m_hWnd;
}

TaskWindow::operator HWND()
{
	return m_hWnd;
}

void TaskWindow::initialize()
{
	string calssName = CreateGUID();
	wstring classNameW = CA2W(calssName.c_str());

	WNDCLASSEX wcex = {};
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WindowProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = GetModuleHandle(NULL);
	wcex.hIcon = NULL;
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = NULL;
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = classNameW.c_str();
	wcex.hIconSm = NULL;
	RegisterClassEx(&wcex);

	HANDLE bCreated = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_LooperThread.reset(new std::thread([this, classNameW, &bCreated] {
		m_hWnd = CreateWindowEx(WS_EX_TOOLWINDOW, classNameW.c_str(), L"", WS_POPUP | WS_VISIBLE, 0, 0,
			0, 0, NULL, NULL, NULL, NULL);

		SetEvent(bCreated);

		MSG msg;
		while (GetMessage(&msg, NULL, 0, 0)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}));

	WaitForSingleObject(bCreated, 1000);
	CloseHandle(bCreated);
}

void TaskWindow::unInit()
{
	if (!::IsWindow(m_hWnd))
		return;

	PostMessage(m_hWnd, WM_CLOSE, 0, 0);
	if (m_LooperThread && m_LooperThread->joinable()) {
		m_LooperThread->join();
		m_LooperThread.reset();
	}
	
}

LRESULT __stdcall TaskWindow::WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if (msg == WM_DESTROY || (msg == WM_CHAR && wparam == VK_RETURN)) {
		PostQuitMessage(0);
		return 0;
	}
	else if (msg == WM_KEYDOWN && wparam == VK_DELETE) {
		app::utils::WindowEnumer::IMAGE_INFO* info = (app::utils::WindowEnumer::IMAGE_INFO*)lparam;

		HDC window_dc = GetWindowDC(hwnd);
		uint8_t* thumbdata = NULL;
		uint32_t width, height;
		if (app::utils::GetWindowImageGDI(hwnd, &thumbdata, width, height)) {
			app::utils::StretchBitmap(window_dc, info->width, info->height, width, height,
				(const char*)thumbdata, info->data);
			delete[] thumbdata;
		}

		ReleaseDC(hwnd, window_dc);
	}

	return DefWindowProcA(hwnd, msg, wparam, lparam);
}
