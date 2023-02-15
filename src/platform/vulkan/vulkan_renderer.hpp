#pragma once
#include "types.hpp"
#include "vulkan.hpp"

class GpuMesh;
struct Transform;
class Logger;
class Window;
#include <chrono>

class VulkanRenderer {
public:
	VulkanRenderer(Window* window, Logger* logger);
	~VulkanRenderer();

	void render(const GpuMesh& mesh, const Transform& transform);
	void set_clear_color(f32 r, f32 g, f32 b, f32 a);
	void begin(bool clear);
	void finish();
private:
	void print_time_between_fn(std::string_view fn);
	std::string last_fn_name {};
	std::chrono::high_resolution_clock::time_point last_fn_end {};

	Window* window;
	Logger* logger;

	vk::DynamicLoader dl {};
	vk::Instance instance;
	vk::SurfaceKHR surface;
	vk::PhysicalDevice phys_device;
	vk::Device device;
	vk::Queue graphics_queue;
	u32 graphics_family;
	vk::Queue transfer_queue;
	u32 transfer_family;

	vk::Extent2D extent;

	constexpr static usize FRAME_COUNT = 2;

	vk::SwapchainKHR swapchain;
	u32 current_frame {};
	u32 image_index {};
	std::vector<vk::Image> images {};
	std::vector<vk::ImageView> image_views {};
	vk::CommandPool graphics_cmd_pool;
	vk::CommandBuffer graphics_cmd_buffers[FRAME_COUNT] {};
	vk::CommandPool transfer_cmd_pool;
	vk::CommandBuffer transfer_cmd_buffer;
	vk::SurfaceFormatKHR format;
	vk::PresentModeKHR mode;
	vk::Semaphore image_acquired_semaphores[FRAME_COUNT] {};
	vk::Semaphore render_finished_semaphores[FRAME_COUNT] {};
	vk::Fence submit_finished_fences[FRAME_COUNT] {};

	vk::ClearColorValue clear_color {};
};