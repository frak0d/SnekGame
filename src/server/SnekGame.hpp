#pragma once

#include <cstdint>
#include <cstdio>
#include <map>
#include <deque>
#include <concepts>
#include <stdexcept>
#include <algorithm>
#include <thread>
#include <utility>

#include <Rand.hpp>
#include <Point.hpp>

using Color = uint32_t;

struct Snek
{
	float angle{}; //relative to +X
	bool boost{false};
	uint16_t score{0};
	const Color color;
	std::deque<Point<float>> parts;
	
	Snek(Color color) : color{color} {}
};

struct SnekGame
{
	Point<float> wrld; // size of the world
	std::map<uint16_t, Snek> snek_list;
	std::vector<Snek> food_list; // dead sneks, pid=0
	
	void addPlayer(uint16_t playerid, Color color)
	{
		auto [elem,done] = snek_list.insert({playerid, Snek{color}});
		//rand_coord({1,1}, wrld)); // initial snek
		elem->second.parts.push_back({1,2});
		elem->second.parts.push_back({1,3});
		elem->second.parts.push_back({1,4});
		elem->second.parts.push_back({1,5});
		elem->second.parts.push_back({1,6});
		elem->second.parts.push_back({1,7});
		elem->second.parts.push_back({1,8});
		elem->second.parts.push_back({1,9});
		puts("player added");
	}

	void delPlayer(uint16_t playerid)
	{
		snek_list.erase(playerid);
		puts("player removed");
	}

	uint16_t getScore(uint16_t playerid)
	{
		return snek_list.at(playerid).score;
	}
	
	uint16_t snekCount()
	{
		return snek_list.size();
	}
	
	const auto& getSnek(uint16_t playerid)
	{
		return snek_list.at(playerid);
	}

	void nextTick()
	{
	for (auto it = snek_list.begin(); it != snek_list.end();)
	{
		bool alive;
		auto& snek = it->second;
		Point<float> move{0.f,0.f};
		
		constexpr float D = 8.f;
		move.x = D*cos(snek.angle);
		move.y = D*sin(snek.angle);
		
		printf("x:%g y:%g dx:%g dy:%g\n", snek.parts[0].x, snek.parts[0].y, move.x, move.y);
		
		if (snek.parts.size() < 5)
			snek.boost = false;
		
		if (snek.boost)
		{
			snek.parts.push_front(snek.parts[0] + move);
			snek.parts.pop_back();
			//collision_check();
			snek.parts.push_front(snek.parts[0] + move);
			snek.parts.pop_back();
			//collision_check();
			
			// Some chance to reduce length on boost
			if (rand() % 10 == 0)
			{
				Snek mini_snek{snek.color};
				mini_snek.parts.push_back(snek.parts.back());
				food_list.push_back(mini_snek);
				snek.parts.pop_back();
			}
		}
		else
		{
			snek.parts.push_front(snek.parts[0] + move);
			snek.parts.pop_back();
		}
		
		for (auto& food : food_list)
		{
			auto eaten_food = std::find(food.parts.begin(), food.parts.end(), snek.parts[0]);
			if (eaten_food != food.parts.end()) // If Food is eaten
			{
				snek.score += 1;
				food.parts.erase(eaten_food);
			}
		}
		
		// Check Boundary Collision
		if (snek.parts[0] < Point<uint16_t>{1,1} or snek.parts[0] > wrld)
		{
			alive = false;
		}
		/*
		// Check Snek Collisions
		else for (const auto& [id,snek2] : snek_list)
		{
			// snek can die by hitting itself as well
			if (std::find(snek2.parts.begin(), snek2.parts.end(), snek.parts[0]) != snek2.parts.end())
			{
				alive = false; break;
			}
		}
		
		if (!alive)
		{
			food_list.push_back(snek);
			it = snek_list.erase(it);
		}
		else*/ ++it;
	}
	}
	
	SnekGame(float x_sz=128, float y_sz=128)
	{
		if (x_sz < 16 or y_sz < 16)
			throw std::runtime_error("World Size too Small !");
		else
			wrld = {x_sz, y_sz};
		
		//std::thread(&SnekGame::food_refiller, this).detach();
	}
};
