#pragma once
#include <cmath>

template<typename T>
struct Vec3 {
	T x, y, z;

	[[nodiscard]] constexpr auto sqr_magnitude() const {
		return x * x + y * y + z * z;
	}

	[[nodiscard]] constexpr auto magnitude() const {
		return std::sqrt(sqr_magnitude());
	}

	[[nodiscard]] constexpr Vec3 normalized() const {
		auto mag = magnitude();
		return {x / mag, y / mag, z / mag};
	}

	constexpr void normalize() {
		auto mag = magnitude();
		x /= mag;
		y /= mag;
		z /= mag;
	}

	constexpr Vec3 operator-(const Vec3& rhs) const {
		return {x - rhs.x, y - rhs.y, z - rhs.z};
	}

	constexpr Vec3 operator+(const Vec3& rhs) const {
		return {x + rhs.x, y + rhs.y, z + rhs.z};
	}

	constexpr Vec3 operator*(const Vec3& rhs) const {
		return {x * rhs.x, y * rhs.y, z * rhs.z};
	}

	constexpr Vec3 operator*(T rhs) const {
		return {x * rhs, y * rhs, z * rhs};
	}

	constexpr Vec3 operator/(const Vec3& rhs) const {
		return {x / rhs.x, y / rhs.y, z / rhs.z};
	}

	constexpr Vec3 operator/(T rhs) const {
		return {x / rhs, y / rhs, z / rhs};
	}
};