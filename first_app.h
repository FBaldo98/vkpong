#pragma once

#include "be_window.h"
#include "be_renderer.h"

namespace be 
{
	
	class FirstApp
	{
	public:
		static constexpr int WIDTH = 800;
		static constexpr int HEIGHT = 600;

		FirstApp() {
			initWindow(window, WIDTH, HEIGHT, "Hello World");

			renderer = new BeRenderer(&window);
			renderer->init();
		}

		void run();

	private:
		BeWindow window = {};
		BeRenderer* renderer;
	};

}