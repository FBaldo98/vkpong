#include "be_window.h"

#include <stdexcept>

namespace be {


	void initWindow(BeWindow& window, int width, int height, std::string windowName)
	{

		window.windowName = windowName;
		window.width = width;
		window.height = height;

		WNDCLASS wc = {};
		wc.hInstance = GetModuleHandle(0);
		wc.lpszClassName = L"be_window";
		wc.lpfnWndProc = ::windProc;
		wc.style = CS_HREDRAW | CS_VREDRAW;

		if (!RegisterClass(&wc)) {
			throw std::runtime_error("Error register WNDCLASS");
		}

		std::wstring tmp = std::wstring(windowName.begin(), windowName.end());
		window.window = CreateWindowEx(WS_EX_APPWINDOW, wc.lpszClassName, tmp.c_str(),
			WS_OVERLAPPEDWINDOW | WS_VISIBLE,
			CW_USEDEFAULT, CW_USEDEFAULT, width, height,
			0, 0,
			GetModuleHandle(0), 0);

		if (window.window == 0)
			throw std::runtime_error("Error creating window");

		::g_running = true;

		ShowWindow(window.window, SW_SHOW);
	}

	void BeWindow::handleMessages()
	{
		MSG msg = {};
		while (PeekMessage(&msg, window, 0, 0, PM_REMOVE) > 0)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
}

LRESULT CALLBACK windProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	LRESULT res = 0;

	switch (msg)
	{
	case WM_CLOSE:
		::g_running = false;
		break;
	case WM_SIZE:
		::g_resized = true;
		break;
	default:
		res = DefWindowProc(wnd, msg, wParam, lParam);
		break;
	}

	return res;

}
