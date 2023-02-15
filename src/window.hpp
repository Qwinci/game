#pragma once
#include <string_view>
#include "types.hpp"
#include <SDL.h>

enum class Platform {
	OpenGL,
	Vulkan
};

class Window {
public:
	Window(std::string_view name, u32 width, u32 height, Platform platform);
	Platform platform;
	SDL_Window* inner;
	u32 width, height;
};