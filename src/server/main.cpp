#include <map>
#include <thread>
#include <ostream>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <csignal>
#include <concepts>
#include <iostream>
#include <algorithm>

#include "SnekGame.hpp"
#include <ixwebsocket/IXWebSocket.h>
#include <ixwebsocket/IXWebSocketServer.h>

using namespace std::literals;
/*
$ HERE ARE PROTOCOL DETAILS :-

[server starts a WebSocket Server]
[all clients connects to the Server]

# NOTE: <-- --> indicates start and end of WS msg

# connection
<--
server (u16 - playerid) >> client
server (x,y world size - u16) >> client
-->

# gameloop
<repeat>
    <--
    server (bool - alive) >> client
    server (u16 - snek_count) >> client
    forall sneks: #let food list be snek id 0
        server (u16 - snek_id) >> client
        server (u16 - num_points) >> client
        server (u32 - color) >> client
        server (u16 - x,y,x,y...) >> client
    -->
    <--
    client (u8 - direction) >> server
    client (bool - boost) >> server
    -->
</repeat>
*/

struct msgbuf
{
    std::string buf;
    
    bool send_to(ix::WebSocket& ws)
    {
        std::cout << std::format("{} bytes\n", buf.size()) << std::endl;
        bool sent = ws.sendBinary(buf).success;
        if (sent) buf.clear();
        return sent;
    }
};

msgbuf& operator<< (msgbuf& msgb, const std::integral auto num)
{
    std::cout << "num:" << +num << std::endl;
    std::string data((char*)&num, (char*)(&num + 1));
    if constexpr (std::endian::native == std::endian::little)
    {
    	std::reverse(data.begin(), data.end());
    }
    for (char ch : data) printf("%02x ", ch & 0xff); puts("");
    msgb.buf += data;
    return msgb;
}

template <typename T>
msgbuf& operator<< (msgbuf& msgb, const Point<T>& pnt)
{
    return msgb << pnt.x << pnt.y;
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

int main()
{
    std::signal(SIGINT, interrupt_handler);
    std::signal(SIGSEGV, segfault_handler);
    
    SnekGame game(256, 256);
    ix::WebSocketServer server(6969);
    std::map<ix::WebSocket*, uint16_t> ws_pid_map;
    std::atomic<uint16_t> pid_gen{0};
    
    server.setOnClientMessageCallback([&](std::shared_ptr<ix::ConnectionState> connectionState,
                                      ix::WebSocket& client, const ix::WebSocketMessagePtr& msg) -> void
    {
        if (msg->type == ix::WebSocketMessageType::Open)
        {
            uint16_t playerid = ++pid_gen;
            
            std::clog << "Connected to" << '\n'
                      << "id: " << playerid << '\n'
                      << "ip: " << connectionState->getRemoteIp() << std::endl;
            
            msgbuf msgb;
            msgb << playerid;
            msgb << game.wrld;
            msgb.send_to(client);
            
            ws_pid_map[&client] = playerid;
            game.addPlayer(playerid, 0xffffffff);
        }
        else if (msg->type == ix::WebSocketMessageType::Close)
        {
            std::clog << "Disconnected from "
                      << connectionState->getRemoteIp() << std::endl;
            
            game.delPlayer(ws_pid_map[&client]);
        }
        else if (msg->type == ix::WebSocketMessageType::Message)
        {
            auto& snek = game.snek_list.at(ws_pid_map[&client]);
            snek.dir = msg->str[0];
            snek.boost = msg->str[1];
            
            std::clog << std::format("id:{}, dir:{}, boost:{}",
                        ws_pid_map[&client],snek.dir,snek.boost) << std::endl;
        }
        else if (msg->type == ix::WebSocketMessageType::Error)
        {
            std::clog << "Connection  Error: " << msg->errorInfo.reason << '\n'
                      << "Retries: " << msg->errorInfo.retries << '\n'
                      << "Wait  time(ms): " << msg->errorInfo.wait_time << '\n'
                      << "HTTP  Status: " << msg->errorInfo.http_status << '\n';
        }
        else if (msg->type == ix::WebSocketMessageType::Ping)
        {
            std::clog << "Received ping from " << connectionState->getRemoteIp() << '\n';
        }
        else
        {
            std::clog << "\n\x1b[33;3m <!> Internal Error: Invalid WebSocketMessageType\x1b[0m\n";
        }
    });
    
    if (!server.listen().first)
    {
        std::puts("\n\x1b[31;1m => Error Initiating WebSocket Server, Exiting...\x1b[0m\n");
        std::exit(-2);
    }
    
    server.start();
    
    while(1)
    {
        game.nextTick(); std::this_thread::sleep_for(250ms);
        
        for (const auto& client_ptr : server.getClients())
        {
            const uint16_t playerid = ws_pid_map[client_ptr.get()];
            
            bool alive;
            try {game.getSnek(playerid); alive=true;}
            catch (...) {alive = false;}
            
            msgbuf msgb;
            msgb << alive;
            msgb << game.snekCount();
            std::cout << "snakes:" << game.snekCount() << std::endl;
            
            for (const auto& [id,snek] : game.snek_list)
            {
                msgb << id;
                msgb << uint16_t(snek.parts.size());
                msgb << snek.color;
                for (const auto& pnt : snek.parts) msgb << pnt;
            }
            msgb.send_to(*client_ptr);
        }
    }
}

/*
 *  EXIT CODES :-
 *  0 -> Normal Exit or Game Over
 * -1 -> Keyboard/System Interrupt
 * -2 -> Error Starting Websocket Server
 * -3 -> <RESERVED>
 * -4 -> Segmentation Fault
 */
