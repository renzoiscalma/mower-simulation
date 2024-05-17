#pragma once
enum class CollisionType {
	NONE = 0,
	OBSTACLE = -1,
	PERIMETER = -2,
	CHARGE_STATION = -3,
	MAP_EDGE = -4, // should not happen!
};