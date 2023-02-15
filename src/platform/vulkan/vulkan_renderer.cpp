#include "vulkan_renderer.hpp"
#include "logger.hpp"
#include "window.hpp"
#include <SDL_vulkan.h>
#include <unordered_set>
#include <chrono>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

VulkanRenderer::VulkanRenderer(Window* window, Logger* logger) : window {window}, logger {logger} {
	logger->log("vulkan", "init begin");

	VULKAN_HPP_DEFAULT_DISPATCHER.init(dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr"));

	vk::ApplicationInfo app_info {
		.pApplicationName = "game",
		.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		.pEngineName = "no engine",
		.engineVersion = VK_MAKE_VERSION(1, 0, 0),
		.apiVersion = VK_API_VERSION_1_3
	};

	auto layer = "VK_LAYER_KHRONOS_validation";

	u32 instance_ext_count;
	if (SDL_Vulkan_GetInstanceExtensions(window->inner, &instance_ext_count, nullptr) == SDL_FALSE) {
		throw std::runtime_error("vulkan: failed to get sdl instance extensions");
	}

	std::vector<const char*> instance_exts(instance_ext_count);
	SDL_Vulkan_GetInstanceExtensions(window->inner, &instance_ext_count, instance_exts.data());

	std::unordered_set<std::string> available_exts;
	for (auto ext : vk::enumerateInstanceExtensionProperties()) {
		available_exts.insert(ext.extensionName);
	}

	for (const auto instance_ext : instance_exts) {
		if (!available_exts.contains(instance_ext)) {
			throw std::runtime_error(std::string("vulkan: required instance extension '") + instance_ext + "' is not supported");
		}
	}

	vk::InstanceCreateInfo instance_info {
		.pApplicationInfo = &app_info,
		.enabledLayerCount = 1,
		.ppEnabledLayerNames = &layer,
		.enabledExtensionCount = instance_ext_count,
		.ppEnabledExtensionNames = instance_exts.data()
	};

	instance = vk::createInstance(instance_info);

	VULKAN_HPP_DEFAULT_DISPATCHER.init(instance);

	logger->log("vulkan", "instance successfully created");

	if (SDL_Vulkan_CreateSurface(window->inner, instance, reinterpret_cast<VkSurfaceKHR*>(&surface)) == SDL_FALSE) {
		instance.destroy();
		throw std::runtime_error("vulkan: sdl surface creation failed");
	}

	auto physical_devices = instance.enumeratePhysicalDevices();

	vk::PhysicalDevice best_physical_device;
	u32 best_graphics_family = 0;
	u32 best_transfer_family = 0;
	usize best_score = 0;

	for (auto phys_dev : physical_devices) {
		usize score = 0;
		auto properties = phys_dev.getProperties();
		if (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
			score += 100000;
		}

		auto queue_families = phys_dev.getQueueFamilyProperties();

		u32 l_graphics_family = UINT32_MAX;
		u32 i_transfer_family = UINT32_MAX;
		bool is_best_graphics_family = false;
		bool is_best_transfer_family = false;

		for (u32 i = 0; i < queue_families.size(); ++i) {
			auto family = queue_families[i];
			auto present_support = phys_dev.getSurfaceSupportKHR(i, surface);
			if (family.queueFlags & vk::QueueFlagBits::eGraphics && present_support) {
				if (!is_best_graphics_family && i_transfer_family != i) {
					is_best_graphics_family = true;
				}
				l_graphics_family = i;
			}
			if (family.queueFlags & vk::QueueFlagBits::eTransfer) {
				if (!is_best_transfer_family && l_graphics_family != i) {
					is_best_transfer_family = true;
				}
				i_transfer_family = i;
			}
		}

		if (l_graphics_family == UINT32_MAX || i_transfer_family == UINT32_MAX) {
			continue;
		}

		if (is_best_graphics_family) {
			score += 1000;
		}
		if (is_best_transfer_family) {
			score += 1000;
		}

		if (score > best_score) {
			best_score = score;
			best_physical_device = phys_dev;
			best_graphics_family = l_graphics_family;
			best_transfer_family = i_transfer_family;
		}
	}

	if (best_score == 0) {
		instance.destroy(surface);
		instance.destroy();
		throw std::runtime_error("vulkan: no suitable gpu was found");
	}

	phys_device = best_physical_device;
	graphics_family = best_graphics_family;
	transfer_family = best_transfer_family;

	auto phys_dev_name = best_physical_device.getProperties().deviceName;
	logger->log("vulkan", std::string("using device '") + phys_dev_name.data() + '\'');

	std::unordered_set<u32> queue_families {best_graphics_family, best_transfer_family};

	std::vector<vk::DeviceQueueCreateInfo> queue_infos;
	queue_infos.reserve(queue_families.size());

	float priority = 1;
	vk::DeviceQueueCreateInfo queue_info {
		.queueCount = 1,
		.pQueuePriorities = &priority
	};

	for (auto f : queue_families) {
		queue_info.queueFamilyIndex = f;
		queue_infos.push_back(queue_info);
	}

	const char* extensions[] {VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME};

	std::unordered_set<std::string> available_device_exts;
	for (auto ext : phys_device.enumerateDeviceExtensionProperties()) {
		available_device_exts.insert(ext.extensionName);
	}

	for (auto ext : extensions) {
		if (!available_device_exts.contains(ext)) {
			instance.destroy(surface);
			instance.destroy();
			throw std::runtime_error(std::string("vulkan: required device extension '") + ext + "' is not supported");
		}
	}

	vk::PhysicalDeviceDynamicRenderingFeatures dynamic_rendering_feature {
		.dynamicRendering = VK_TRUE
	};

	vk::DeviceCreateInfo device_info {
		.pNext = &dynamic_rendering_feature,
		.queueCreateInfoCount = as<uint32_t>(queue_infos.size()),
		.pQueueCreateInfos = queue_infos.data(),
		.enabledExtensionCount = sizeof(extensions) / sizeof(*extensions),
		.ppEnabledExtensionNames = extensions
	};

	device = phys_device.createDevice(device_info);

	graphics_queue = device.getQueue(graphics_family, 0);
	transfer_queue = device.getQueue(transfer_family, 0);

	vk::SurfaceFormatKHR best_format {vk::Format::eUndefined};
	vk::PresentModeKHR best_mode = vk::PresentModeKHR::eFifo;

	auto supported_formats = phys_device.getSurfaceFormatsKHR(surface);
	auto supported_modes = phys_device.getSurfacePresentModesKHR(surface);

	for (const auto& f : supported_formats) {
		if (f.format == vk::Format::eR8G8B8A8Srgb && f.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
			best_format = f;
			break;
		}
	}

	if (best_format.format == vk::Format::eUndefined) {
		best_format = supported_formats[0];
	}

	format = best_format;

	for (auto m : supported_modes) {
		if (m == vk::PresentModeKHR::eMailbox) {
			best_mode = m;
			break;
		}
	}

	mode = best_mode;

	auto caps = phys_device.getSurfaceCapabilitiesKHR(surface);

	u32 image_count = FRAME_COUNT;
	if (image_count < caps.minImageCount) {
		image_count = caps.minImageCount;
	}
	if (caps.maxImageCount && image_count > caps.maxImageCount) {
		image_count = caps.maxImageCount;
	}

	extent.width = std::clamp(extent.width, caps.minImageExtent.width, caps.maxImageExtent.width);
	extent.height = std::clamp(extent.height, caps.minImageExtent.height, caps.maxImageExtent.height);

	vk::SwapchainCreateInfoKHR swapchain_info {
		.surface = surface,
		.minImageCount = image_count,
		.imageFormat = format.format,
		.imageColorSpace = format.colorSpace,
		.imageExtent = extent,
		.imageArrayLayers = 1,
		.imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
		.imageSharingMode = vk::SharingMode::eExclusive,
		.preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity,
		.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
		.presentMode = mode,
		.clipped = VK_TRUE
	};

	vk::ImageViewCreateInfo image_view_info {
		.viewType = vk::ImageViewType::e2D,
		.format = format.format,
		.subresourceRange {
			.aspectMask = vk::ImageAspectFlagBits::eColor,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1
		}
	};

	swapchain = device.createSwapchainKHR(swapchain_info);
	images = device.getSwapchainImagesKHR(swapchain);
	for (auto& image : images) {
		image_view_info.image = image;
		image_views.push_back(device.createImageView(image_view_info));
	}

	extent = {window->width, window->height};

	vk::CommandPoolCreateInfo cmd_pool_info {
		.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
		.queueFamilyIndex = graphics_family
	};
	graphics_cmd_pool = device.createCommandPool(cmd_pool_info);

	cmd_pool_info.queueFamilyIndex = transfer_family;
	transfer_cmd_pool = device.createCommandPool(cmd_pool_info);

	vk::CommandBufferAllocateInfo cmd_buffer_info {
		.commandPool = graphics_cmd_pool,
		.level = vk::CommandBufferLevel::ePrimary,
		.commandBufferCount = 1
	};

	for (auto& cmd_buf : graphics_cmd_buffers) {
		cmd_buf = device.allocateCommandBuffers(cmd_buffer_info)[0];
	}

	cmd_buffer_info.commandPool = transfer_cmd_pool;
	transfer_cmd_buffer = device.allocateCommandBuffers(cmd_buffer_info)[0];

	vk::SemaphoreCreateInfo semaphore_info {};

	for (auto& semaphore : image_acquired_semaphores) {
		semaphore = device.createSemaphore(semaphore_info);
	}

	for (auto& semaphore : render_finished_semaphores) {
		semaphore = device.createSemaphore(semaphore_info);
	}

	vk::FenceCreateInfo fence_info {
		.flags = vk::FenceCreateFlagBits::eSignaled
	};

	for (auto& fence : submit_finished_fences) {
		fence = device.createFence(fence_info);
	}

	vk::ImageMemoryBarrier barrier {
		.srcAccessMask = vk::AccessFlagBits::eNone,
		.dstAccessMask = vk::AccessFlagBits::eNone,
		.oldLayout = vk::ImageLayout::eUndefined,
		.newLayout = vk::ImageLayout::ePresentSrcKHR,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.subresourceRange {
			.aspectMask = vk::ImageAspectFlagBits::eColor,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1
		}
	};

	vk::CommandBufferBeginInfo begin_info {
		.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit
	};

	graphics_cmd_buffers[0].begin(begin_info);

	for (auto& image : images) {
		barrier.image = image;

		graphics_cmd_buffers[0].pipelineBarrier(
				vk::PipelineStageFlagBits::eAllGraphics,
				vk::PipelineStageFlagBits::eAllGraphics,
				{},
				{},
				{},
				{barrier}
				);
	}

	graphics_cmd_buffers[0].end();

	vk::SubmitInfo submit_info {
		.commandBufferCount = 1,
		.pCommandBuffers = &graphics_cmd_buffers[0],
	};

	graphics_queue.submit({submit_info});

	graphics_queue.waitIdle();

	logger->log("vulkan", "renderer init done");
}

void VulkanRenderer::render(const GpuMesh& mesh, const Transform& transform) {

}

void VulkanRenderer::set_clear_color(f32 r, f32 g, f32 b, f32 a) {
	clear_color = vk::ClearColorValue {{{r, g, b, a}}};
}

#include <iostream>

namespace chrono = std::chrono;

void VulkanRenderer::begin(bool clear) {
	vk::CommandBufferBeginInfo cmd_begin_info {
			.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit
	};

	if (device.waitForFences({submit_finished_fences[current_frame]}, VK_TRUE, UINT64_MAX) != vk::Result::eSuccess) {
		logger->log("vulkan", "failed to wait for submit fence", LogLevel::Warn);
	}
	print_time_between_fn("waitForFences");

	device.resetFences({submit_finished_fences[current_frame]});
	print_time_between_fn("resetFences");

	auto res = device.acquireNextImageKHR(swapchain, UINT64_MAX, image_acquired_semaphores[current_frame]);
	if (res.result == vk::Result::eErrorOutOfDateKHR || res.result == vk::Result::eSuboptimalKHR) {
		//logger->log("vulkan", "using suboptimal or out of date swapchain", LogLevel::Warn);
	}
	print_time_between_fn("acquireNextImageKHR");
	image_index = res.value;

	vk::RenderingAttachmentInfo color_attachment_info {
			.imageView = image_views[image_index],
			.imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
			.loadOp = vk::AttachmentLoadOp::eClear,
			.storeOp = vk::AttachmentStoreOp::eStore,
	};
	if (clear) {
		color_attachment_info.clearValue.color = clear_color;
	}

	const vk::RenderingInfo render_info {
			.renderArea {.extent = extent},
			.layerCount = 1,
			.colorAttachmentCount = 1,
			.pColorAttachments = &color_attachment_info
	};


	graphics_cmd_buffers[current_frame].reset();
	print_time_between_fn("cmd_buf_reset");

	graphics_cmd_buffers[current_frame].begin(cmd_begin_info);
	print_time_between_fn("cmd_buf_begin");

	/*const vk::ImageMemoryBarrier image_start_barrier {
		.srcAccessMask = vk::AccessFlagBits::e
		.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite,
		.oldLayout = vk::ImageLayout::eUndefined,
		.newLayout = vk::ImageLayout::eColorAttachmentOptimal,
		.image = images[image_index],
		.subresourceRange {
				.aspectMask = vk::ImageAspectFlagBits::eColor,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1
		}
	};

	graphics_cmd_buffers[current_frame].pipelineBarrier(
			vk::PipelineStageFlagBits::eTopOfPipe,
			vk::PipelineStageFlagBits::eColorAttachmentOutput,
			{},
			{},
			{},
			{image_start_barrier}
	);
	print_time_between_fn("pipelineBarrier (start)");*/

	//graphics_cmd_buffers[current_frame].beginRendering(render_info);
	//print_time_between_fn("beginRendering");
}

void VulkanRenderer::finish() {
	//graphics_cmd_buffers[current_frame].endRendering();
	//print_time_between_fn("endRendering");

	const vk::ImageMemoryBarrier image_barrier {
		.srcAccessMask = vk::AccessFlagBits::eNone,
		.dstAccessMask = vk::AccessFlagBits::eNone,
		.oldLayout = vk::ImageLayout::eUndefined,
		.newLayout = vk::ImageLayout::ePresentSrcKHR,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = images[image_index],
		.subresourceRange {
			.aspectMask = vk::ImageAspectFlagBits::eColor,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1
		}
	};

	/*graphics_cmd_buffers[current_frame].pipelineBarrier(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,
			vk::PipelineStageFlagBits::eBottomOfPipe,
			{},
			{},
			{},
			{image_barrier});
	print_time_between_fn("pipelineBarrier (end)");*/

	graphics_cmd_buffers[current_frame].end();
	print_time_between_fn("cmd_buf_end");

	vk::PipelineStageFlags wait_stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;

	vk::SubmitInfo submit_info {
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &image_acquired_semaphores[current_frame],
		.pWaitDstStageMask = &wait_stage,
		.commandBufferCount = 1,
		.pCommandBuffers = &graphics_cmd_buffers[current_frame],
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &render_finished_semaphores[current_frame]
	};

	graphics_queue.submit(submit_info, submit_finished_fences[current_frame]);
	print_time_between_fn("submit");

	vk::PresentInfoKHR present_info {
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &render_finished_semaphores[current_frame],
		.swapchainCount = 1,
		.pSwapchains = &swapchain,
		.pImageIndices = &image_index
	};

	if (graphics_queue.presentKHR(present_info) != vk::Result::eSuccess) {
		logger->log("vulkan", "failed to present image", LogLevel::Warn);
	}

	print_time_between_fn("presentKHR");

	current_frame = (current_frame + 1) % FRAME_COUNT;
}

VulkanRenderer::~VulkanRenderer() {
	device.waitIdle();

	for (auto& semaphore : image_acquired_semaphores) {
		device.destroy(semaphore);
	}

	for (auto& semaphore : render_finished_semaphores) {
		device.destroy(semaphore);
	}

	for (auto& fence : submit_finished_fences) {
		device.destroy(fence);
	}

	device.destroy(graphics_cmd_pool);
	device.destroy(transfer_cmd_pool);
	for (auto& view : image_views) {
		device.destroy(view);
	}
	device.destroy(swapchain);
	device.destroy();
	instance.destroy(surface);
	instance.destroy();
}

void VulkanRenderer::print_time_between_fn(std::string_view fn) {
	auto now = chrono::high_resolution_clock::now();
	auto diff = chrono::duration_cast<chrono::milliseconds>(now - last_fn_end);
	last_fn_end = now;
	if (diff.count() > 0) {
		logger->log("vulkan", std::string("time between ")
		+ last_fn_name + " and " + fn.data() + " is " + std::to_string(diff.count()) + "ms");
	}
	last_fn_name = fn;
}
