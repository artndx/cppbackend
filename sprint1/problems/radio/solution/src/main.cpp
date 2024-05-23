#define WIN32_LEAN_AND_MEAN
#include "audio.h"
#include <boost/asio.hpp>
#include <array>
#include <iostream>
#include <string>
#include <string_view>

namespace net = boost::asio;
using net::ip::udp;
using namespace std::literals;

// void RecordingVoice(){
//     Recorder recorder(ma_format_u8, 1);
//     Player player(ma_format_u8, 1);

//     while (true) {
//         std::string str;

//         std::cout << "Press Enter to record message..." << std::endl;
//         std::getline(std::cin, str);

//         auto rec_result = recorder.Record(65000, 1.5s);
//         std::cout << "Recording done" << std::endl;

//         player.PlayBuffer(rec_result.data.data(), rec_result.frames, 1.5s);
//         std::cout << "Playing done" << std::endl;
//     }
// }

void Server(std::string port){
    try{
        net::io_context io;
        udp::socket socket(io, udp::endpoint(udp::v4(), std::stoi(port)));

        while(true){
            std::vector<char> buff(60000);
            udp::endpoint remote_endpoint;

            Player player(ma_format_u8, 1);
            auto required_bytes_count = socket.receive_from(net::buffer(buff), remote_endpoint);
            player.PlayBuffer(buff.data(), buff.size() / player.GetFrameSize(), 1.5s);
            std::cout << "Playing done" << std::endl;
        }
    } catch(std::exception& e){
        std::cerr << e.what() << '\n';
    }
}

void Client(std::string ip_address, std::string port){
    try {
        net::io_context io;
        udp::socket socket(io, udp::v4());

        Recorder recorder(ma_format_u8, 1);
        std::string str;
        std::cout << "Press Enter to record message..." << std::endl;
        std::getline(std::cin, str);
        auto rec_result = recorder.Record(65000, 1.5s);
        std::cout << "Recording done" << std::endl;
        
        boost::system::error_code ec;
        auto endpoint = udp::endpoint(net::ip::make_address(ip_address, ec), std::stoi(port));
        socket.send_to(net::buffer(rec_result.data), endpoint);
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
}

int main(int argc, char** argv) {
    if (argc != 4) {
        std::cout << "Usage: "sv << argv[0] << " <client/server> <server IP> <port>"sv << std::endl;
        return 1;
    }

    if(argv[1] == "client"s){
        Client(argv[2], argv[3]);
    } else if(argv[1] == "server"s) {
        Server(argv[3]);
    } else {
        std::cout << "Incorrect argument\n";
    }
}
