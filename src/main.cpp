#include "window.hpp"
#include "renderer.hpp"
#include "logger.hpp"

int main() {
	Window window {"game", 800, 600, Platform::Vulkan};
	Logger logger {};
	Renderer renderer {&window, Platform::Vulkan, &logger};

	renderer.set_clear_color(0, 1, 0, 1);

	bool running = true;
	while (running) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) {
				running = false;
			}
		}

		renderer.begin(true);
		renderer.finish();
	}
}