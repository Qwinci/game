#include "renderer.hpp"
#include "logger.hpp"

Renderer::Renderer(Window* window, Platform platform, Logger* logger) : platform {platform} { // NOLINT(cppcoreguidelines-pro-type-member-init)
	try {
		switch (platform) {
			case Platform::Vulkan:
				new (&vulkan_renderer) VulkanRenderer {window, logger};
				break;
			case Platform::OpenGL:
				opengl_renderer = OpenGlRenderer {};
				break;
		}
	}
	catch (const std::exception& e) {
		logger->log("render", e.what(), LogLevel::Error);
		exit(1);
	}
}

void Renderer::render(const GpuMesh& mesh, const Transform& transform) {
	switch (platform) {
		case Platform::Vulkan:
			vulkan_renderer.render(mesh, transform);
			break;
		case Platform::OpenGL:
			opengl_renderer.render(mesh, transform);
			break;
	}
}

void Renderer::set_clear_color(f32 r, f32 g, f32 b, f32 a) {
	switch (platform) {
		case Platform::Vulkan:
			vulkan_renderer.set_clear_color(r, g, b, a);
			break;
		case Platform::OpenGL:
			opengl_renderer.set_clear_color(r, g, b, a);
			break;
	}
}

void Renderer::begin(bool clear) {
	switch (platform) {
		case Platform::Vulkan:
			vulkan_renderer.begin(clear);
			break;
		case Platform::OpenGL:
			opengl_renderer.begin(clear);
			break;
	}
}

void Renderer::finish() {
	switch (platform) {
		case Platform::Vulkan:
			vulkan_renderer.finish();
			break;
		case Platform::OpenGL:
			opengl_renderer.finish();
			break;
	}
}

Renderer::~Renderer() {
	switch (platform) {
		case Platform::Vulkan:
			vulkan_renderer.~VulkanRenderer();
			break;
		case Platform::OpenGL:
			opengl_renderer.~OpenGlRenderer();
			break;
	}
}
