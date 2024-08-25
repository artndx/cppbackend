#include <boost/json.hpp>
#include "model.h"
#include <random>
#include <sstream>
#include <optional>
#include <functional>

namespace app{

namespace json = boost::json;

using namespace model;

namespace detail {
struct TokenTag {};
}  // namespace detail

using Token = util::Tagged<std::string, detail::TokenTag>;


class Players;
class PlayerTokens;

/* ------------------------ Player ----------------------------------- */

class Player{
public:
    using Name = util::Tagged<std::string, Player>;
    int GetId() const{
        return id_;
    }

    Name GetName() const{
        return name_;
    }

    Dog* GetDog(){
        return dog_;
    }

    const Dog* GetDog() const{
        return static_cast<const Dog*>(dog_);
    }

    const GameSession* GetSession() const{
        return session_;
    }
private:
    friend PlayerTokens;
    friend Players;

    Player(int id, Name name, Dog* dog, const GameSession* session)
        : id_(id), name_(name), dog_(dog), session_(session){
    }

    int id_;
    Name name_;
    Dog* dog_;
    const GameSession* session_;
};

/* ------------------------ Players ----------------------------------- */

class Players{
public:

    using DogMapKey = std::pair<int, Map::Id>;
    struct DogMapKeyHasher{
        size_t operator()(const DogMapKey& value) const;
    };
    using PlayerList = std::unordered_map<DogMapKey, Player, DogMapKeyHasher>;
    Players() = default;

    Player& Add(int id, const Player::Name& name, Dog* dog, const GameSession* session);

    const Player* FindByDogIdAndMapId(int dog_id, std::string map_id) const;

    const PlayerList& GetPlayers() const;
private:
    PlayerList players_;
};

/* ---------------------- PlayerTokens ------------------------------------- */

class PlayerTokens{
public:
    PlayerTokens() = default;

    Token AddPlayer(Player& player);

    Player* FindPlayerByToken(const Token& token); 

    const Player* FindPlayerByToken(const Token& token) const;
private:
    Token GenerateToken();
    std::random_device random_device_;

    std::mt19937_64 generator1_{[this] {
        std::uniform_int_distribution<std::mt19937_64::result_type> dist;
        return dist(random_device_);
    }()};

    std::mt19937_64 generator2_{[this] {
        std::uniform_int_distribution<std::mt19937_64::result_type> dist;
        return dist(random_device_);
    }()};
    // Чтобы сгенерировать токен, получите из generator1_ и generator2_
    // два 64-разрядных числа и, переведя их в hex-строки, склейте в одну.
    // Вы можете поэкспериментировать с алгоритмом генерирования токенов,
    // чтобы сделать их подбор ещё более затруднительным
    std::unordered_map<Token, Player*, util::TaggedHasher<Token>> token_to_player_;
};

/* ------------------------ Use Cases ----------------------------------- */

class GetMapUseCase{
public:
    static std::string MakeMapDescription(const model::Map* map);
private:
    static json::array GetRoadsInJSON(const model::Map::Roads& all_roads);
    static json::array GetBuildingsInJSON(const model::Map::Buildings& buildings);
    static json::array GetOfficesInJSON(const model::Map::Offices& offices);
};

class ListMapsUseCase{
public:
    static std::string MakeMapsList(const model::Game::Maps& maps);
};

class GameUseCase{
public:
    GameUseCase(app::Players& players, app::PlayerTokens& tokens)
        : players_(players), tokens_(tokens){}

    std::string JoinGame(const std::string& user_name, const std::string& str_map_id, 
                            model::Game& game, bool random_spawn);

    std::string GetGameState() const;

    std::string SetAction(const json::object& action, const Token& token);

    std::string IncreaseTime(double delta, model::Game& game);
private:
    model::Dog::Position GetFirstPos(const model::Map::Roads& roads) const;
    model::Dog::Position GetRandomPos(const model::Map::Roads& roads) const;
    int auto_counter_ = 0;
    app::Players& players_;
    app::PlayerTokens& tokens_;
};

class ListPlayersUseCase{
public:
    static std::string GetPlayersInJSON(const app::Players::PlayerList& players);
};
/* --------------------------- Application -------------------------------- */

class Application{
public:
    Application(model::Game& game, bool periodic_mode, bool randomize_spawn_points)
        : 
        game_(game), periodic_mode_(periodic_mode), 
        rand_spawn_(randomize_spawn_points), players_(), tokens_(), 
        game_handler_(players_, tokens_){}

    std::string GetMapsList(){
        return ListMapsUseCase::MakeMapsList(game_.GetMaps());
    }

    const Map* FindMap(const Map::Id& map_id){
        return game_.FindMap(map_id);
    }

    const Player* FindPlayerByToken(const Token& token){
        return tokens_.FindPlayerByToken(token);
    }

    bool IsPeriodicMode() const{
        return periodic_mode_;
    }

    std::string GetMapDescription(const model::Map* map){
        return GetMapUseCase::MakeMapDescription(map);
    }

    std::string GetJoinGameResult(const std::string& user_name, const std::string& map_id){
        return game_handler_.JoinGame(user_name, map_id, game_, rand_spawn_);
    }

    std::string GetPlayerList(){
        return ListPlayersUseCase::GetPlayersInJSON(players_.GetPlayers());
    }

    std::string GetGameState(){
        return game_handler_.GetGameState();
    }

    std::string IncreaseTime(double delta){
        return game_handler_.IncreaseTime(delta, game_);
    }

    std::string ApplyPlayerAction(const json::object& action, const Token& token){
        return game_handler_.SetAction(action, token);
    }
private:
    model::Game& game_;
    bool periodic_mode_;
    bool rand_spawn_;
    Players players_;
    PlayerTokens tokens_; 
    GameUseCase game_handler_;
};

} // namespace app