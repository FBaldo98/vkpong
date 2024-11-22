
#include "first_app.h"

#define NOMINMAX
#include <Windows.h>

#include <cstdlib>
#include <iostream>
#include <stdexcept>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR cmdLine, int cmdShow)
{

#ifdef _DEBUG
	AllocConsole();
	freopen("CONIN%", "r", stdin);
	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);
	std::cout.sync_with_stdio();
#endif // _DEBUG


	be::FirstApp app;

	try
	{
		app.run();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << "\n";
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}