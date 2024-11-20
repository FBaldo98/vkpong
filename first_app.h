#pragma once

#include "be_window.h"

namespace be 
{
	
	class FirstApp
	{
	public:
		static constexpr int WIDTH = 800;
		static constexpr int HEIGHT = 600;

		FirstApp() {
			initWindow(window, WIDTH, HEIGHT, "Hello World");
		}

		void run();

	private:
		BeWindow window = {};
	};

}