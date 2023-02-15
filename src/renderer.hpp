#pragma once
#include "window.hpp"
#include "platform/vulkan/vulkan_renderer.hpp"
#include "platform/opengl/opengl_renderer.hpp"

class GpuMesh;
struct Transform;
class Logger;

class Renderer {
public:
	Renderer(Window* window, Platform platform, Logger* logger);
	~Renderer();
	void render(const GpuMesh& mesh, const Transform& transform);
	void set_clear_color(f32 r, f32 g, f32 b, f32 a);
	void begin(bool clear);
	void finish();
private:
	Platform platform;

	union {
		VulkanRenderer vulkan_renderer;
		OpenGlRenderer opengl_renderer;
	};
};