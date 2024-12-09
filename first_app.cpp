#include "first_app.h"

namespace be {

	void FirstApp::run()
	{
		while (::g_running) {
			window.handleMessages();
			renderer->drawFrame();
		}
	}

}