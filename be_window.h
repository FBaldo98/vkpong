#pragma once

#define NOMINMAX
#include <Windows.h>

#include<string>

inline bool g_running = false;

LRESULT CALLBACK windProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam);


namespace be {

	struct BeWindow
	{
		int width;
		int height;
		std::string windowName;

		HWND window = {};

		void handleMessages();
	};

	void initWindow(BeWindow& window, int width, int height, std::string windowName);

}