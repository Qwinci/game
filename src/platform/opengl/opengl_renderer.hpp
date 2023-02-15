#pragma once
#include "types.hpp"

class GpuMesh;
struct Transform;

class OpenGlRenderer {
public:
	OpenGlRenderer();

	void render(const GpuMesh& mesh, const Transform& transform);
	void set_clear_color(f32 r, f32 g, f32 b, f32 a);
	void begin(bool clear);
	void finish();
};