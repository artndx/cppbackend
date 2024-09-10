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
    static std::string MakeMapDescription(const Map* map);
private:
    static json::array GetRoadsInJSON(const Map::Roads& roads);
    static json::array GetBuildingsInJSON(const Map::Buildings& buildings);
    static json::array GetOfficesInJSON(const Map::Offices& offices);
    static json::array GetLootTypesInJSON(const Map::LootTypes& loot_types);
};

class ListMapsUseCase{
public:
    static std::string MakeMapsList(const Game::Maps& maps);
};

class GameUseCase{
public:
    GameUseCase(Players& players, PlayerTokens& tokens)
        : players_(players), tokens_(tokens){}

    std::string JoinGame(const std::string& user_name, const std::string& str_map_id, 
                            Game& game, bool is_random_spawn_enabled);

    std::string GetGameState(const Token& token) const;

    std::string SetAction(const json::object& action, const Token& token);

    std::string IncreaseTime(double delta, Game& game);

    void GenerateLoot(detail::Milliseconds delta, Game& game);
private:
    json::object GetPlayers(const GameSession* session) const;
    json::object GetLostObjects(const GameSession* session) const;

    int auto_counter_ = 0;
    Players& players_;
    PlayerTokens& tokens_;
};

class ListPlayersUseCase{
public:
    static std::string GetPlayersInJSON(const PlayerTokens::PlayersInSession& players);
};
/* --------------------------- Application -------------------------------- */

class Application{
public:
    Application(Game& game, bool periodic_mode, bool randomize_spawn_points)
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

    const Player* FindPlayerByToken(const Token& token) const{
        return tokens_.FindPlayerByToken(token);
    }

    bool IsPeriodicMode() const{
        return periodic_mode_;
    }

    std::string GetMapDescription(const Map* map) const{
        return GetMapUseCase::MakeMapDescription(map);
    }

    std::string GetJoinGameResult(const std::string& user_name, const std::string& map_id){
        return game_handler_.JoinGame(user_name, map_id, game_, rand_spawn_);
    }

    std::string GetPlayerList(const Token& token) const{
        const GameSession* session = tokens_.FindPlayerByToken(token)->GetSession();
        PlayerTokens::PlayersInSession players = tokens_.GetPlayersBySession(session);
        return ListPlayersUseCase::GetPlayersInJSON(players);
    }

    std::string GetGameState(const Token& token) const{
        return game_handler_.GetGameState(token);
    }

    std::string IncreaseTime(double delta){
        return game_handler_.IncreaseTime(delta, game_);
    }

    void GenerateLoot(detail::Milliseconds delta){
        return game_handler_.GenerateLoot(delta, game_);
    }

    std::string ApplyPlayerAction(const json::object& action, const Token& token){
        return game_handler_.SetAction(action, token);
    }
private:
    Game& game_;
    bool periodic_mode_;
    bool rand_spawn_;
    Players players_;
    PlayerTokens tokens_; 
    GameUseCase game_handler_;
};

} // namespace app