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
	uint8_t dir{0};
	bool boost{false};
	uint16_t score{0};
	const Color color;
	std::deque<Point<uint16_t>> parts;
	
	Snek(Color color) : color{color} {}
};

struct SnekGame
{
	Point<uint16_t> wrld; // size of the world
	std::map<uint16_t, Snek> snek_list;
	std::vector<Snek> food_list; // dead sneks, pid=0
	
	void addPlayer(uint16_t playerid, Color color)
	{
		auto [elem,done] = snek_list.insert({playerid, Snek{color}});
		elem->second.parts.push_back(rand_coord({1,1}, wrld)); // initial snek
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
		auto& snek = it->second;
		Point<int16_t> move{0,0};
		bool alive;
		
		switch (snek.dir) //0 means no move
		{
			break;case 4: move = {-1, 0}; //left
			break;case 6: move = { 1, 0}; //right
			break;case 8: move = { 0,-1}; //top
			break;case 2: move = { 0, 1}; //bottom
			break;case 7: move = {-1,-1}; //top-left
			break;case 9: move = { 1,-1}; //top-right
			break;case 1: move = {-1, 1}; //bottom-left
			break;case 3: move = { 1, 1}; //bottom right
		}
		
		if (snek.boost)
		{
			snek.parts.push_front(snek.parts[0] + move);
			snek.parts.push_front(snek.parts[0] + move);
			snek.parts.pop_back();
			snek.parts.pop_back();
			snek.parts.pop_back();
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
		else ++it;
	}
	}
	
	SnekGame(uint16_t x_sz=128, uint16_t y_sz=128)
	{
		if (x_sz < 16 or y_sz < 16)
			throw std::runtime_error("World Size too Small !");
		else
			wrld = {x_sz, y_sz};
		
		//std::thread(&SnekGame::food_refiller, this).detach();
	}
};
