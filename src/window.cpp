#include "window.hpp"
#include <SDL.h>

Window::Window(std::string_view name, u32 width, u32 height, Platform platform)
	: platform {platform}, width {width}, height {height} {
	static bool init = false;

	if (!init) {
		SDL_Init(SDL_INIT_VIDEO);
		init = true;
	}

	SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0");

	auto sdl_platform = platform == Platform::OpenGL ? SDL_WINDOW_OPENGL : SDL_WINDOW_VULKAN;
	inner = SDL_CreateWindow(
			name.data(),
			SDL_WINDOWPOS_CENTERED,
			SDL_WINDOWPOS_CENTERED,
			as<int>(width),
			as<int>(height),
			SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE | sdl_platform);
}
