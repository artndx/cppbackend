#ifdef WIN32
#include <sdkddkver.h>
#endif

#include "seabattle.h"

#include <atomic>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <iostream>
#include <optional>
#include <string>
#include <thread>
#include <string_view>

namespace net = boost::asio;
using net::ip::tcp;
using namespace std::literals;

void PrintFieldPair(const SeabattleField& left, const SeabattleField& right) {
    auto left_pad = "  "s;
    auto delimeter = "    "s;
    std::cout << left_pad;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << delimeter;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << std::endl;
    for (size_t i = 0; i < SeabattleField::field_size; ++i) {
        std::cout << left_pad;
        left.PrintLine(std::cout, i);
        std::cout << delimeter;
        right.PrintLine(std::cout, i);
        std::cout << std::endl;
    }
    std::cout << left_pad;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << delimeter;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << std::endl;
}

template <size_t sz>
static std::optional<std::string> ReadExact(tcp::socket& socket) {
    boost::array<char, sz> buf;
    boost::system::error_code ec;

    net::read(socket, net::buffer(buf), net::transfer_exactly(sz), ec);

    if (ec) {
        return std::nullopt;
    }

    return {{buf.data(), sz}};
}

static bool WriteExact(tcp::socket& socket, std::string_view data) {
    boost::system::error_code ec;

    net::write(socket, net::buffer(data), net::transfer_exactly(data.size()), ec);

    return !ec;
}

class SeabattleAgent {
public:
    SeabattleAgent(const SeabattleField& field)
        : my_field_(field) {
    }

    void StartGame(tcp::socket& socket, bool my_initiative) {
        PrintFields();

        // First move
        bool result_move;
        if(my_initiative){
            // Start Client
            result_move = MakeMove(socket);

        } else {
            // Start Server
            result_move = WaitMove(socket);
        }

        while(!IsGameEnded()){
            if(result_move){
                result_move = MakeMove(socket);
            } else {
                result_move = WaitMove(socket);
            }
        }

        if(my_field_.IsLoser()){
            std::cout << "### You win !!! ###\n";
        } else {
            std::cout << "&&& You lose !!! &&&n";
        }
    }

private:

    static std::optional<std::pair<int, int>> ParseMove(const std::string_view& sv) {
        if (sv.size() != 2) return std::nullopt;

        int p1 = sv[0] - 'A', p2 = sv[1] - '1';

        if (p1 < 0 || p1 > 8) return std::nullopt;
        if (p2 < 0 || p2 > 8) return std::nullopt;

        return {{p1, p2}};
    }

    static std::string MoveToString(std::pair<int, int> move) {
        char buff[] = {static_cast<char>(move.first) + 'A', static_cast<char>(move.second) + '1'};
        return {buff, 2};
    }

    void PrintFields() const {
        PrintFieldPair(my_field_, other_field_);
    }

    bool IsGameEnded() const {
        return my_field_.IsLoser() || other_field_.IsLoser();
    }

    // TODO: добавьте методы по вашему желанию

    bool MakeMove(tcp::socket& socket){
        std::cout << "You turn: ";
        
        /* Write */
        std::string move;
        std::cin >> move;
        auto atacked_cell = ParseMove(move);
        
        if(!WriteExact(socket, move)){
            std::cout << "Error sending data"sv << std::endl;
        }

        /* Read */
        auto attack_result = ReadExact<1>(socket);
        if (!attack_result.has_value()) {
            std::cout << "Error reading data"sv << std::endl;
        }
        std::string result_message;
        int attack_result_code = std::stoi(*attack_result);
        switch (attack_result_code)
        {
        case 0:
            result_message = "Miss!";
            if(atacked_cell.has_value()){
                other_field_.MarkMiss((*atacked_cell).second, (*atacked_cell).first);
            }
            break;
        case 1:
            result_message = "Hit!";
            if(atacked_cell.has_value()){
                other_field_.MarkHit((*atacked_cell).second, (*atacked_cell).first);
            }
            break;
        case 2:
            result_message = "Kill!";
            if(atacked_cell.has_value()){
                other_field_.MarkKill((*atacked_cell).second, (*atacked_cell).first);
            }
            break;
        
        default:
            break;
        }
        std::cout << result_message << std::endl;
        PrintFields();
        return (attack_result_code != 0) ? true : false;
    }

    bool WaitMove(tcp::socket& socket){
        std::cout << "Waiting for turn..." << std::endl;

        /* Read */
        auto move = ReadExact<2>(socket);
        if (!move.has_value()) {
            std::cout << "Error reading data"sv << std::endl;
        }
        std::cout << "Shot to " << *move << std::endl;
        
        /* Write */
        auto attacked_cell = ParseMove(*move);
        SeabattleField::ShotResult result = SeabattleField::ShotResult::MISS;
        if(attacked_cell.has_value()){
            result = my_field_.Shoot((*attacked_cell).second, (*attacked_cell).first);
            switch (result)
            {
            case SeabattleField::ShotResult::MISS:
                my_field_.MarkMiss((*attacked_cell).second, (*attacked_cell).first);
                break;
            case SeabattleField::ShotResult::HIT:
                my_field_.MarkHit((*attacked_cell).second, (*attacked_cell).first);
                break;
            case SeabattleField::ShotResult::KILL:
                my_field_.MarkKill((*attacked_cell).second, (*attacked_cell).first);
                break;
            default:
                break;
            }
        } else {
            std::cout << "There is incorrect attacked_cell" << std::endl;
        }

        std::string result_data = std::to_string(int(result));
        
        if(!WriteExact(socket, result_data));
        PrintFields();
        return (result == SeabattleField::ShotResult::MISS) ? true : false;
    }

private:
    SeabattleField my_field_;
    SeabattleField other_field_;
};

void StartServer(const SeabattleField& field, unsigned short port) {
    SeabattleAgent agent(field);

    net::io_context io;
    tcp::acceptor acceptor(io, tcp::endpoint(tcp::v4(), port));
    std::cout << "Waiting for connection..."sv << std::endl;
    boost::system::error_code ec;
    tcp::socket socket{io};
    acceptor.accept(socket, ec);
    if(ec){
        std::cout << "Can't accept connection"sv << std::endl;
        return;
    }
    agent.StartGame(socket, false);
};

void StartClient(const SeabattleField& field, const std::string& ip_str, unsigned short port) {
    SeabattleAgent agent(field);

    boost::system::error_code ec;
    auto endpoint = tcp::endpoint(net::ip::make_address(ip_str, ec), port);
    if (ec) {
        std::cout << "Wrong IP format"sv << std::endl;
        return;
    }
    net::io_context io;
    tcp::socket socket{io};
    socket.connect(endpoint, ec);
    if (ec) {
        std::cout << "Can't connect to server"sv << std::endl;
        return;
    }
    agent.StartGame(socket, true);
};

int main(int argc, const char** argv) {
    if (argc != 3 && argc != 4) {
        std::cout << "Usage: program <seed> [<ip>] <port>" << std::endl;
        return 1;
    }

    std::mt19937 engine(std::stoi(argv[1]));
    SeabattleField fieldL = SeabattleField::GetRandomField(engine);

    if (argc == 3) {
        StartServer(fieldL, std::stoi(argv[2]));
    } else if (argc == 4) {
        StartClient(fieldL, argv[2], std::stoi(argv[3]));
    }
}
