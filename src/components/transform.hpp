#pragma once
#include "types.hpp"

struct Transform {
	Vec3<f32> position {0, 0, 0};
	Vec3<f32> rotation {0, 0, 0};
	Vec3<f32> scale {1, 1, 1};
};