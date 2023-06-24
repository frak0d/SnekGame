#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <bit>
#include <cmath>
#include <cstdint>
#include <format>
#include <math.h>
#include <ostream>
#include <stddef.h>
#include <cstdlib>
#include <utility>
#include <stdio.h>
#include <stdint.h>
#include <complex>
#include <future>
#include <thread>
#include <vector>
#include <deque>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <csignal>
#include <concepts>
#include <iostream>
#include <algorithm>
#include <type_traits>

#include <Point.hpp>
#include <Serial.hpp>

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocket.h>

using namespace std::literals;

struct msg_buf : public iSerial, public oSerial
{
    ix::WebSocket& ws;
    
    msg_buf(ix::WebSocket& WS) : ws{WS} {};
    
    bool send() //sends out_buffer as a single websocket message
    {
        bool sent = ws.sendBinary(out_buffer.data()).success;
        if (sent) out_buffer.clear();
        return sent;
    }
};

// SIGNAL HANDLERS //

void interrupt_handler(int)
{
    std::puts("\n\x1b[31;1m => Interrrupt Recieved, Exiting...\x1b[0m\n");
    std::exit(-1);
}

void segfault_handler(int)
{
    std::puts("\n\x1b[31;1m => Segmentation Fault, Exiting...\x1b[0m\n");
    std::exit(-4);
}

struct Snek : public sf::Drawable
{
    uint16_t id;
    sf::Color color;
    std::deque<Point<float>> parts;
    
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override
    {
        bool x = 0;
        for (int i=parts.size()-1 ; i >= 0 ; --i)
        {
            x =! x;
            sf::CircleShape circle(8,32);
            
            circle.setOrigin(8,8);
            circle.setPosition(parts[i].x, parts[i].y);
            circle.setFillColor(x ? color : sf::Color::White);
            
            target.draw(circle, states);
        }
    }
};

struct Food : public Snek
{
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override
    {
        for (int i=0 ; i < parts.size() ; ++i)
        {
            sf::CircleShape circle(4,3);
            
            circle.setOrigin(4,4);
            circle.setPosition(parts[i].x, parts[i].y);
            circle.setFillColor(color);
            
            target.draw(circle, states);
        }
    }
};

int main()
{
    std::signal(SIGINT, interrupt_handler);
    std::signal(SIGSEGV, segfault_handler);
    
    float angle{};
    bool boost{0};
    
    bool alive;
    uint16_t playerid;
    Point<float> wrld;
    Point<float> my_head;
    
    std::mutex net_mtx;
    std::vector<Snek> snek_list;
    std::vector<Food> food_list;
    
    //////////////////////////////////////////////////////////////////////////////////////
    
    ix::initNetSystem();
    std::atexit([]{ix::uninitNetSystem();});
    
    ix::WebSocket ws;
    msg_buf server{ws};
    ws.setUrl("ws://3.110.83.3:6969");
    
    bool first_time = true;
    
    ws.setOnMessageCallback([&](const ix::WebSocketMessagePtr& msg)
    {
        if (msg->type == ix::WebSocketMessageType::Open)
        {
            puts("Connection Established!");
        }
        else if (msg->type == ix::WebSocketMessageType::Close)
        {
            std::clog << "Disconnected: " << msg->closeInfo.reason << '\n';
        }
        else if (msg->type == ix::WebSocketMessageType::Message)
        {
            server.in_buffer.append(msg->str);
            
            snek_list.clear(); food_list.clear();
            
            if (first_time)
            {
                first_time = false;
                
                server >> playerid;
                server >> wrld;
                
                std::clog << "pid:" << playerid << '\n'
                          << "wrld:"<< wrld.x << ',' << wrld.y << std::endl;
            }
            else
            {
                net_mtx.lock();
            /*
                server (bool - alive) >> client
                server (u16 - snek_count) >> client
                forall sneks: #let food list be snek id 0
                    server (u16 - snek_id) >> client
                    server (u16 - num_points) >> client
                    server (u32 - color) >> client
                    server (u16 - x,y,x,y...) >> client
            */
                server >> alive;
                uint16_t snek_count;
                server >> snek_count;
                
                while (snek_count--)
                {
                    uint16_t id;
                    server >> id;
                    
                    auto make = [&](Snek* body)
                    {
                        body->id = id;
                        
                        uint16_t num_points;
                        server >> num_points;
                        
                        uint32_t rgba;
                        server >> rgba;
                        body->color = sf::Color(rgba);
                        
                        Point<float> pnt;
                        while (num_points--)
                        {
                            server >> pnt;
                            body->parts.push_back(pnt);
                        }
                    };
                    
                    if (id == 0)
                    {
                        Food food; make(&food);
                        food_list.push_back(food);
                    }
                    else
                    {
                        Snek snek; make(&snek);
                        snek_list.push_back(snek);
                    }
                }
                net_mtx.unlock();
            /*
                client (u8 - direction) >> server
                client (bool - boost) >> server
            */
                server << angle;
                server << boost;
                server.send();
            }
        }
        else if (msg->type == ix::WebSocketMessageType::Error)
        {
            std::clog << "Connection  Error: " << msg->errorInfo.reason << '\n'
                      << "Retries: " << msg->errorInfo.retries << '\n'
                      << "Wait  time(ms): " << msg->errorInfo.wait_time << '\n'
                      << "HTTP  Status: " << msg->errorInfo.http_status << '\n';
        }
        else
        {
            std::clog << "\n\x1b[33;3m <!> Internal Error: Invalid WebSocketMessageType\x1b[0m\n";
        }
    });
    ws.start();
    
    //////////////////////////////////////////////////////////////////////////////////////
    
    sf::ContextSettings context;
    context.antialiasingLevel = 4;
    
    sf::View view({0.f, 0.f, 1000.f, 750.f});
    
    sf::RenderWindow window(sf::VideoMode(1000, 750), "Score : 0",
                            sf::Style::Default, context);    
    window.setView(view);
    window.setFramerateLimit(60);
    
    sf::Music music;
    music.setLoop(true);
    
    if (music.openFromFile("music-loop.ogg"))
        music.play();
    
    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
            {
                ws.stop(); window.close();
            }
            
            else if (event.type == sf::Event::Resized)
            {
                view.setSize(event.size.width, event.size.height);
                window.setView(view);
            }
            
            else if (event.type == sf::Event::KeyPressed)
            {
                if (event.key.code == sf::Keyboard::Escape)
                {
                    ws.stop(); window.close();
                }
            }
            
            else if (event.type == sf::Event::MouseButtonPressed)
            {
                if (event.mouseButton.button == sf::Mouse::Right)
                    boost = true;
            }
            
            else if (event.type == sf::Event::MouseButtonReleased)
            {
                if (event.mouseButton.button == sf::Mouse::Right)
                    boost = false;
            }
            
            else if (event.type == sf::Event::MouseMoved)
            {
                if (sf::Mouse::isButtonPressed(sf::Mouse::Left))
                {
                    auto pos = window.mapPixelToCoords({event.mouseMove.x, event.mouseMove.y});
                    
                    float dx = (pos.x - my_head.x);
                    float dy = (pos.y - my_head.y);
                    
                    angle = atan2(dy,dx);
                }
            }
        }
        
        window.clear();
        
        sf::RectangleShape border({wrld.x, wrld.y});
        border.setFillColor(sf::Color::Black);
        border.setOutlineColor(sf::Color::White);
        border.setOutlineThickness(5);
        window.draw(border);
        
        sf::RectangleShape marker({5.f,5.f});
        marker.setOrigin({2.5f, 2.5f});
        marker.setFillColor(sf::Color::Black);
        marker.setOutlineColor(sf::Color(76, 0, 128));
        marker.setOutlineThickness(1);
        
        for (float x=64 ; x < wrld.x ; x += 64)
        {
            for (float y=64 ; y < wrld.y ; y += 64)
            {
                marker.setPosition(x,y);
                window.draw(marker);
            }
        }
        
        if (!first_time) //once the connection is done
        {
            net_mtx.lock();
            
            for (const auto& food : food_list) window.draw(food);
            
            for (const auto& snek : snek_list)
            {
                if (snek.id == playerid)
                {
                    if (snek.parts.size()) my_head = snek.parts[0];
                    int pos = snek.parts.size()/2; if (pos > 20) pos = 20;
                    view.setCenter(snek.parts[pos-1].x, snek.parts[pos-1].y); window.setView(view);
                    window.setTitle(std::format("Score : {}", snek.parts.size()*100));
                }
                window.draw(snek);
            }
            
            net_mtx.unlock();
        }
        
        window.display();
    }
}