#pragma once

#include <random>
#include <concepts>
#include "Point.hpp"

std::random_device rndm_dev;   // these 2 lines are outside the
std::mt19937 rndm_mt(rndm_dev()); // fuction to increase performance

template <typename T> requires std::floating_point<T> or std::integral<T>
Point<T> rand_coord(const Point<T>& low_lim, const Point<T>& up_lim)
{
	if constexpr (std::is_floating_point_v<T>)
	{
		std::uniform_real_distribution<T> xgen(low_lim.x, up_lim.x);
		std::uniform_real_distribution<T> ygen(low_lim.y, up_lim.y);
		
		T x = xgen(rndm_mt);
		T y = ygen(rndm_mt);
		
		return {x,y};
	}
	else if constexpr (std::is_integral_v<T>)
	{
		std::uniform_int_distribution<T> xgen(low_lim.x, up_lim.x);
		std::uniform_int_distribution<T> ygen(low_lim.y, up_lim.y);

		T x = xgen(rndm_mt);
		T y = ygen(rndm_mt);
		
		return {x,y};
	}
}
