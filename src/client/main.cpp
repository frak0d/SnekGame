
#include <SFML/Window/Keyboard.hpp>
#include <bit>
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
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocket.h>

using namespace std::literals;

struct msg_buf
{
    ix::WebSocket* ws;
    std::string in_msg_buf;
    std::string out_msg_buf;
    
    // MUST be called Before using the singleton
    void bind_websocket(ix::WebSocket* ws_ptr) {ws=ws_ptr;};
    
    bool send() //sends the buffer as a single websocket message
    {
        assert(ws); //check if not null
        bool sent = ws->sendBinary(out_msg_buf).success;
        if (sent) out_msg_buf.clear();
        return sent;
    }
    
    std::string get_data(size_t bytes=0)
    {
        if (bytes == 0)
            while (in_msg_buf.empty())
                std::this_thread::sleep_for(10ms);
        else
            while (in_msg_buf.size() < bytes)
                std::this_thread::sleep_for(1ms);
        
        std::string data;
        
        data = in_msg_buf.substr(0, bytes);
        in_msg_buf.erase(0, bytes);
        
        return data;
    }
} server;

// SENDING TO WEBSOCKET //

msg_buf& operator << (msg_buf& msgb, const std::integral auto& num)
{
    std::string data((char*)&num, (char*)(&num + 1));
    if constexpr (std::endian::native == std::endian::little)
    {
        std::reverse(data.begin(), data.end());
    }
    msgb.out_msg_buf += data;
    return msgb;
}

template <typename T>
msg_buf& operator << (msg_buf& msgb, const Point<T>& pnt)
{
    return msgb << pnt.x << pnt.y;
}

// RECIEVING FROM WEBSOCKET //

msg_buf& operator >> (msg_buf& msgb, std::integral auto& num)
{
    std::string data = msgb.get_data(sizeof(num));
    if constexpr (std::endian::native == std::endian::little)
    {
        std::reverse(data.begin(), data.end());
    }
    std::cout << data.size() << std::endl;
    for (char ch : data) printf("%02x ", ch & 0xff); puts("");
    num = *reinterpret_cast<decltype(&num)>(data.data());
    std::cout << "num:" << +num << std::endl;
    return msgb;
}

template <typename T>
msg_buf& operator >> (msg_buf& msgb, Point<T>& pnt)
{
    return msgb >> pnt.x >> pnt.y;
}

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
    std::deque<Point<uint16_t>> parts;
    
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override
    {
        bool x = 0;
        for (int i=0 ; i < parts.size() ; ++i)
        {
            x =! x; // stripes
            sf::RectangleShape rect({4.f, 4.f});
            
            rect.setOrigin(rect.getSize()/2.f);
            rect.setPosition(parts[i].x*5, parts[i].y*5);
            rect.setFillColor(x ? sf::Color::Yellow : color);
            
            if (i > 0)
            {
                //diagonal movement check
                if ((parts[i].x != parts[i-1].x) and (parts[i].y != parts[i-1].y))
                    rect.setRotation(45);
            }
            
            target.draw(rect, states);
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
            
            circle.setOrigin(6,6);
            circle.setPosition(parts[i].x*5, parts[i].y*5);
            circle.setFillColor(color);
            
            target.draw(circle, states);
        }
    }
};

int main()
{
    std::signal(SIGINT, interrupt_handler);
    std::signal(SIGSEGV, segfault_handler);
    
    bool boost;
    uint8_t move;
    
    bool alive;
    uint16_t playerid;
    Point<uint16_t> wrld;
    
    std::mutex net_mtx;
    std::vector<Snek> snek_list;
    std::vector<Food> food_list;
    
    //////////////////////////////////////////////////////////////////////////////////////
    
    ix::initNetSystem();
    std::atexit([]{ix::uninitNetSystem();});
    
    ix::WebSocket ws;
    ws.setUrl("ws://localhost:6969");
    server.bind_websocket(&ws);
    
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
            std::cout << std::format("{} bytes\n", msg->str.size()) << std::endl;
            server.in_msg_buf.append(msg->str);
            
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
                std::cout << "sneks:" << snek_count <<std::endl;
                while (snek_count--)
                {
                    uint16_t id;
                    server >> id;
                    
                    auto make = [&](Snek* body)
                    {
                        body->id = id;
                        
                        uint16_t num_points;
                        server >> num_points;
                        
                std::cout << "num_points:" << num_points <<std::endl;
                        
                        uint32_t rgba;
                        server >> rgba;
                        body->color = sf::Color(rgba);
                        
                        Point<uint16_t> pnt;
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
                server << move;
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
    
    sf::RenderWindow win(sf::VideoMode(1280, 800), "Score : 0", sf::Style::Close);
    win.setFramerateLimit(30);
    
    sf::Music music;
    music.setLoop(true);
    if (music.openFromFile("music-loop.ogg"))
        music.play();
    
    while (win.isOpen())
    {
        sf::Event event;
        while (win.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
            {
                ws.stop();
                win.close();
            }
            
            else if (event.type == sf::Event::KeyPressed) switch (event.key.code)
            {
                break;case sf::Keyboard::Numpad1: move = 1;
                break;case sf::Keyboard::Numpad2: move = 2;
                break;case sf::Keyboard::Numpad3: move = 3;
                break;case sf::Keyboard::Numpad4: move = 4;
                break;case sf::Keyboard::Numpad5: boost=true;
                break;case sf::Keyboard::Numpad6: move = 6;
                break;case sf::Keyboard::Numpad7: move = 7;
                break;case sf::Keyboard::Numpad8: move = 8;
                break;case sf::Keyboard::Numpad9: move = 9;
                break;case sf::Keyboard::Escape: std::exit(0);
                break;default:break;
            }
            
            else if (event.type == sf::Event::KeyReleased)
                if (event.key.code == sf::Keyboard::Escape)
                    boost = false;
        }
        
        win.clear();
        
        if (!first_time) //once the connection is done
        {
            net_mtx.lock();
            
            for (const auto& food : food_list) win.draw(food);
            
            for (const auto& snek : snek_list)
            {
                win.draw(snek);
                if (snek.id == playerid)
                {
                    win.setTitle(std::format("Score : {}", snek.parts.size()*100));
                    //TODO: set camera center to snek.parts[0]
                }
            }
            
            net_mtx.unlock();
        }
        
        win.display();
    }
}