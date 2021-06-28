#pragma once
#include <thread>
#include <string>
#include <memory>
#include <Windows.h>

class TaskWindow
{
public:
	TaskWindow();
	~TaskWindow();

	HWND GetView();

	operator HWND ();
private:
	void initialize();
	void unInit();
	static LRESULT WINAPI WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

private:
	HWND m_hWnd;
	std::unique_ptr<std::thread> m_LooperThread;
};

