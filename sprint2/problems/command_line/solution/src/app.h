#pragma once

#include <boost/json.hpp>
#include "player.h"
#include <sstream>
#include <optional>
#include <functional>

namespace app{

namespace json = boost::json;

using namespace model;

/* ------------------------ Use Cases ----------------------------------- */

class GetMapUseCase{
public:
    static std::string MakeMapDescription(const model::Map* map);
private:
    static json::array GetRoadsInJSON(const model::Map::Roads& roads);
    static json::array GetBuildingsInJSON(const model::Map::Buildings& buildings);
    static json::array GetOfficesInJSON(const model::Map::Offices& offices);
};

class ListMapsUseCase{
public:
    static std::string MakeMapsList(const model::Game::Maps& maps);
};

class GameUseCase{
public:
    GameUseCase(model::Players& players, model::PlayerTokens& tokens)
        : players_(players), tokens_(tokens){}

    std::string JoinGame(const std::string& user_name, const std::string& str_map_id, 
                            model::Game& game, bool random_spawn);

    std::string GetGameState() const;

    std::string SetAction(const json::object& action, const model::Token& token);

    std::string IncreaseTime(double delta, model::Game& game);
private:
    model::Dog::Position GetFirstPos(const model::Map::Roads& roads) const;
    model::Dog::Position GetRandomPos(const model::Map::Roads& roads) const;
    int auto_counter_ = 0;
    model::Players& players_;
    model::PlayerTokens& tokens_;
};

class ListPlayersUseCase{
public:
    static std::string GetPlayersInJSON(const model::Players::PlayerList& players);
};
/* --------------------------- Application -------------------------------- */

class Application{
public:
    Application(model::Game& game, bool periodic_mode, bool randomize_spawn_points)
        : 
        game_(game), periodic_mode_(periodic_mode), 
        rand_spawn_(randomize_spawn_points), players_(), tokens_(), 
        game_handler_(players_, tokens_){}

    std::string GetMapsList() const{
        return ListMapsUseCase::MakeMapsList(game_.GetMaps());
    }

    const Map* FindMap(const Map::Id& map_id) const{
        return game_.FindMap(map_id);
    }

    const Player* FindPlayerByToken(const model::Token& token) const{
        return tokens_.FindPlayerByToken(token);
    }

    bool IsPeriodicMode() const{
        return periodic_mode_;
    }

    std::string GetMapDescription(const model::Map* map) const{
        return GetMapUseCase::MakeMapDescription(map);
    }

    std::string GetJoinGameResult(const std::string& user_name, const std::string& map_id){
        return game_handler_.JoinGame(user_name, map_id, game_, rand_spawn_);
    }

    std::string GetPlayerList() const{
        return ListPlayersUseCase::GetPlayersInJSON(players_.GetPlayers());
    }

    std::string GetGameState() const{
        return game_handler_.GetGameState();
    }

    std::string IncreaseTime(double delta){
        return game_handler_.IncreaseTime(delta, game_);
    }

    std::string ApplyPlayerAction(const json::object& action, const model::Token& token){
        return game_handler_.SetAction(action, token);
    }
private:
    model::Game& game_;
    bool periodic_mode_;
    bool rand_spawn_;
    model::Players players_;
    model::PlayerTokens tokens_; 
    GameUseCase game_handler_;
};

} // namespace app