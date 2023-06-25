#pragma once

#include <chrono>
#include <complex>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <iomanip>
#include <map>
#include <deque>
#include <concepts>
#include <mutex>
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
	Color color;
	std::deque<Point<float>> parts;
	
	Snek(Color color) : color{color} {}
};

struct SnekGame
{
	std::mutex mtx;
	Point<float> wrld; // size of the world
	std::map<uint16_t, Snek> snek_list;
	std::vector<Snek> food_list; // dead sneks, pid=0
	
	void addPlayer(uint16_t playerid, Color color)
	{
		auto [elem,done] = snek_list.insert({playerid, Snek{color}});
		//rand_coord({1,1}, wrld)); // initial snek
		for (int i=0; i<10; ++i)
		{
			elem->second.parts.push_back({1024.f,1024.f});
		}
	}
	
	void delPlayer(uint16_t playerid)
	{
		snek_list.erase(playerid);
		puts("player removed");
	}
	
	uint16_t snekCount()
	{
		return snek_list.size() + food_list.size();
	}
	
	const auto& getSnek(uint16_t playerid)
	{
		return snek_list.at(playerid);
	}

	void nextTick()
	{
		std::erase_if(snek_list, [&](auto& pair)
		{
			bool alive{1};
			auto& snek = pair.second;
			auto playerid = pair.first;
			
			Point<float> move {8*cosf(snek.angle), 8*sinf(snek.angle)};
			
			auto collision_check = [&]
			{
				for (auto food = food_list.begin(); food != food_list.end();)
				{
					for (auto part = food->parts.begin(); part != food->parts.end();)
					{
						float dx = part->x - snek.parts[0].x;
						float dy = part->y - snek.parts[0].y;
						
						// snek radius is 8, food radius is 12
						// => collision distance = 8 + 12 = 20
						
						if (dx*dx + dy*dy < 20*20)
						{
							snek.parts.push_back(snek.parts.back());
							part = food->parts.erase(part);
						}
						else ++part;
					}
					
					if (food->parts.empty())
						food = food_list.erase(food);
					else
						++food;
				}
				
				// Check Boundary Collision
				if (snek.parts[0] < Point<float>{0.f,0.f} or snek.parts[0] > wrld)
				{
					alive = false;
					return;
				}
				
				// Check Inter-Snek Collisions
				else for (const auto& [id2,snek2] : snek_list)
				{
					// self intersection allowed
					if (id2 == playerid) continue;
					
					if (alive) for (const auto& part : snek2.parts)
					{
						float dx = part.x - snek.parts[0].x;
						float dy = part.y - snek.parts[0].y;
						
						// snek part radius is 8
						// => collision distance = 16
						
						if (dx*dx + dy*dy < 16*16)
						{
							alive = false; return;
						}
					}
				}
			};
			
			////////////////////////////////////////////////////
			
			size_t food_count{0};
			for (const auto& food : food_list)
				food_count += food.parts.size();
			
			if (food_count < wrld.x/10 && rand() % 100)
			{
				Snek newfood{uint32_t(rand()) | 0xff};
				for (int i=0 ; i < 8 ; ++i)
					newfood.parts.push_back(rand_coord({0.f,0.f}, wrld));
				food_list.push_back(newfood);
			}
			
			////////////////////////////////////////////////////
			
			if (snek.parts.size() < 5)
				snek.boost = false;
			
			if (snek.boost)
			{
				snek.parts.push_front(snek.parts[0] + move);
				snek.parts.pop_back();
				
				collision_check();
				if (!alive) goto dead_snek;
				
				snek.parts.push_front(snek.parts[0] + move);
				snek.parts.pop_back();
				
				collision_check();
				if (!alive) goto dead_snek;
				
				// Some chance to reduce length on boost
				if (rand() % 12 == 0)
				{
					if (rand() % 2 == 0) // only 50% is recycled
					{
						Snek mini_snek{snek.color};
						mini_snek.parts.push_back(snek.parts.back());
						food_list.push_back(mini_snek);
					}
					snek.parts.pop_back();
				}
			}
			else
			{
				snek.parts.push_front(snek.parts[0] + move);
				snek.parts.pop_back();
				
				collision_check();
				if (!alive) goto dead_snek;
			}
			
			return false; //keep
			
		dead_snek:
			food_list.push_back(snek);
			return true; //erase
		});
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
