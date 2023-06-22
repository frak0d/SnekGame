#pragma once

#include <iostream>
#include <type_traits>

template <typename T=int>
requires std::is_arithmetic_v<T>
struct Point
{
	T x,y;
	
	bool operator == (const Point& rhs) const
	{
		return (x == rhs.x) && (y == rhs.y);
	}
	
	bool operator < (const Point& rhs) const
	{
		return (x < rhs.x) || (y < rhs.y);
	}
	
	bool operator > (const Point& rhs) const
	{
		return (x > rhs.x) || (y > rhs.y);
	}
	
	bool operator << (const Point& rhs) const
	{
		return (x < rhs.x) && (y < rhs.y);
	}
	
	bool operator >> (const Point& rhs) const
	{
		return (x > rhs.x) && (y > rhs.y);
	}
	
	// Arithmatic Operators //

	template <typename T2>
	Point<T> operator + (const Point<T2>& rhs)
	{
		return {T(x+rhs.x), T(y+rhs.y)};
	}
	
	template <typename T2>
	Point<T> operator - (const Point<T2>& rhs)
	{
		return {T(x-rhs.x), T(y-rhs.y)};
	}
};
