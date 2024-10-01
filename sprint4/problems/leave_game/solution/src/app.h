#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/json.hpp>
#include <pqxx/pqxx>
#include <chrono>
#include <sstream>
#include <optional>
#include <functional>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <fstream>
#include "player.h"
#include "model_serialization.h"
#include "global.h"

namespace app{

namespace json = boost::json;
namespace net = boost::asio;
namespace sys = boost::system;
using Strand = net::strand<net::io_context::executor_type>;
using Clock = std::chrono::steady_clock;
using pqxx::operator""_zv;
using namespace std::literals;
using namespace model::detail;
using namespace model;

namespace detail{

/* ------------------------ Ticker ----------------------------------- */

class Ticker : public std::enable_shared_from_this<Ticker> {
public:
    using Handler = std::function<void(Milliseconds delta)>;
    
    // Функция handler будет вызываться внутри strand с интервалом period
    Ticker(Strand& strand, Milliseconds period, Handler handler)
        : strand_{strand}
        , period_{period}
        , handler_{std::move(handler)} {
    }

    void Start();

private:
    void ScheduleTick();

    void OnTick(sys::error_code ec);


    Strand& strand_;
    Milliseconds period_;
    net::steady_timer timer_{strand_};
    Handler handler_;
    Clock::time_point last_tick_;
};

/* ------------------------ PlayerTimeClock ----------------------------------- */

/* Класс для отслеживания за бездействием игрока и его игровым временем*/
class PlayerTimeClock{
public:
    PlayerTimeClock() = default;

    void IncreaseTime(size_t delta);

    std::optional<Milliseconds> GetInactivityTime() const;

    void UpdateActivity(Dog::Speed new_speed);

    Milliseconds GetPlaytime() const;
private:
    Clock::time_point log_in_time_ = Clock::now();
    Clock::time_point current_playtime = Clock::now();
    std::optional<Clock::time_point> inactivity_start_time_ = Clock::now();
    std::optional<Clock::time_point> current_inactivity_time_;
};

/* ------------------------ ConnectionPool ----------------------------------- */

class ConnectionPool {
public:
    using PoolType = ConnectionPool;
    using ConnectionPtr = std::shared_ptr<pqxx::connection>;
    class ConnectionWrapper {
    public:
        ConnectionWrapper(std::shared_ptr<pqxx::connection>&& conn, PoolType& pool) noexcept
            : conn_{std::move(conn)}
            , pool_{&pool} {
        }

        ConnectionWrapper(const ConnectionWrapper&) = delete;
        ConnectionWrapper& operator=(const ConnectionWrapper&) = delete;

        ConnectionWrapper(ConnectionWrapper&&) = default;
        ConnectionWrapper& operator=(ConnectionWrapper&&) = default;

        pqxx::connection& operator*() const& noexcept {
            return *conn_;
        }
        pqxx::connection& operator*() const&& = delete;

        pqxx::connection* operator->() const& noexcept {
            return conn_.get();
        }

        ~ConnectionWrapper() {
            if (conn_) {
                pool_->ReturnConnection(std::move(conn_));
            }
        }

    private:
        std::shared_ptr<pqxx::connection> conn_;
        PoolType* pool_;
    };

    // ConnectionFactory is a functional object returning std::shared_ptr<pqxx::connection>
    template <typename ConnectionFactory>
    ConnectionPool(size_t capacity, ConnectionFactory&& connection_factory){
        pool_.reserve(capacity);
        for (size_t i = 0; i < capacity; ++i) {
            pool_.emplace_back(connection_factory());
        }
    }

    ConnectionWrapper GetConnection();

private:
    void ReturnConnection(ConnectionPtr&& conn);

    std::mutex mutex_;
    std::condition_variable cond_var_;
    std::vector<ConnectionPtr> pool_;
    size_t used_connections_ = 0;
};

/* ------------------------ ConnectionFactory ----------------------------------- */

inline ConnectionPool::ConnectionPtr ConnectionFactory(){
    auto conn = std::make_shared<pqxx::connection>(DB_URL);
    conn->prepare("insert", R"(
                        INSERT INTO retired_players (name, score, time) VALUES ($1, $2, $3);
                        )");
    conn->prepare("select", R"(
                        SELECT name, score, time FROM retired_players 
                        ORDER BY score, time, name DESC 
                        LIMIT $1 
                        OFFSET $2;
                        )");
    return conn;
}

} // namespace detail

/* ------------------------ Use Cases ----------------------------------- */

/* ------------------------ GetMapUseCase ----------------------------------- */


class GetMapUseCase{
public:
    static std::string MakeMapDescription(const Map* map);
private:
    static json::array GetRoadsInJSON(const Map::Roads& roads);
    static json::array GetBuildingsInJSON(const Map::Buildings& buildings);
    static json::array GetOfficesInJSON(const Map::Offices& offices);
    static json::array GetLootTypesInJSON(const Map::LootTypes& loot_types);
};

/* ------------------------ ListMapsUseCase ----------------------------------- */

class ListMapsUseCase{
public:
    static std::string MakeMapsList(const Game::Maps& maps);
};

/* ------------------------ GameUseCase ----------------------------------- */

class GameUseCase{
public:
    using PlayerTimeClocks = std::unordered_map<const Player*, detail::PlayerTimeClock>;
    
    GameUseCase(Players& players, PlayerTokens& tokens)
        : players_(players), tokens_(tokens), connection_pool_(NUM_THREADS, detail::ConnectionFactory){
            auto conn = connection_pool_.GetConnection();
            pqxx::work work{*conn};

            // work.exec(R"(
            //     DROP TABLE IF EXISTS retired_players;
            //     )"_zv);

            work.exec(R"(
                CREATE TABLE IF NOT EXISTS retired_players (
                    id SERIAL PRIMARY KEY,
                    name varchar(100) NOT NULL,
                    score integer NOT NULL,
                    time real NOT NULL
                );
                )"_zv);
            work.exec(R"(
                    CREATE INDEX IF NOT EXISTS score_time_name_idx ON retired_players (score, time, name DESC);
            )");
            work.commit();
        }

    std::string JoinGame(const std::string& user_name, const std::string& str_map_id, 
                            Game& game, bool is_random_spawn_enabled);

    std::string GetGameState(const Token& token) const;

    std::string SetAction(const json::object& action, const Token& token);

    std::string IncreaseTime(unsigned delta, Game& game);

    static void GenerateLoot(Milliseconds delta, Game& game);

    std::string GetRecords(unsigned start, unsigned max_items);
private:
    static json::array GetBagItems(const Dog::Bag& bag_items);
    static json::object GetPlayers(const PlayerTokens::PlayersInSession& players_in_session);
    static json::object GetLostObjects(const std::deque<Loot>& loots);
    void AddPlayerTimeClock(Player* player);
    void SaveScore(const Player* player);
    void DisconnectPlayer(const Player* player, Game& game);

    int auto_counter_ = 0;
    Players& players_;
    PlayerTokens& tokens_;
    PlayerTimeClocks clocks_;
    detail::ConnectionPool connection_pool_;
};

/* ------------------------ ListPlayersUseCase ----------------------------------- */

class ListPlayersUseCase{
public:
    static std::string GetPlayersInJSON(const PlayerTokens::PlayersInSession& players);
};

/* ------------------------ GameStateSaveCase ----------------------------------- */

class GameStateSaveCase{
public:
    GameStateSaveCase(std::string state_file, std::optional<unsigned> period, const Game::SessionsByMapId& sessions, const Players& players)
    : state_file_(state_file), 
    save_state_period_(period),
    sessions_(sessions),
    players_(players),
    last_tick_(Clock::now()){}

    void SaveOnTick(bool is_periodic){
        if(save_state_period_.has_value()){
            if(is_periodic){
                Clock::time_point this_tick = Clock::now();
                auto delta = std::chrono::duration_cast<Milliseconds>(this_tick - last_tick_);
                if(delta >= FromInt(save_state_period_.value())){
                    SaveState();
                    last_tick_ = Clock::now(); 
                }
            } else {
                SaveState();
            }
        }
    }

    void SaveState(){
        using namespace std::literals;
        std::fstream fstrm(state_file_ /*+ "_temp"s*/, std::ios::out);
        boost::archive::text_oarchive output_archive{fstrm};
        serialization::GameStateRepr writed_game_state(sessions_, players_);
        output_archive << writed_game_state;
    }

    serialization::GameStateRepr LoadState(){
        using namespace std::literals;
        std::fstream fstrm(state_file_, std::ios::in);
        serialization::GameStateRepr game_state;
        try{
            boost::archive::text_iarchive input_archive{fstrm};
            input_archive >> game_state;
            return game_state;
        } catch(...){
            return game_state;
        }
    }

private:
    Clock::time_point last_tick_;
    std::string state_file_;
    std::optional<unsigned> save_state_period_; 
    const Game::SessionsByMapId& sessions_;
    const Players& players_;
};

/* --------------------------- Application -------------------------------- */

class Application{
public:

    Application(Game& game, 
                Strand api_strand, 
                std::optional<unsigned> tick_period, 
                std::optional<std::string> state_file, 
                std::optional<unsigned> save_state_period,
                bool randomize_spawn_points)
        : 
        game_(game), 
        api_strand_(api_strand),
        tick_period_(tick_period), 
        rand_spawn_(randomize_spawn_points), players_(), tokens_(), 
        game_handler_(players_, tokens_), time_ticker_(), loot_ticker_(){
            /* Перед началом работы приложения всегда генерируется начальный лут*/
            GenerateLoot(Milliseconds{0});

            /* 
                Если в аргументах командной строки 
                указан период обновления игрового состояния,
                то создаются таймер на обновление игрового состояния 
                и таймер на обновления лута
            */
            if(tick_period_.has_value()){
                time_ticker_ = std::make_shared<detail::Ticker>(api_strand_, FromInt(*tick_period_), [this](Milliseconds delta){
                    this->IncreaseTime(delta.count() / 1000);
                });

                time_ticker_->Start();

                loot_ticker_ = std::make_shared<detail::Ticker>(api_strand_, game_.GetLootGeneratePeriod(), [this](Milliseconds delta){
                    this->GenerateLoot(delta);
                });

                loot_ticker_->Start();
            }

            if(state_file.has_value()){
                state_save_.emplace(state_file.value(), save_state_period, game_.GetAllSessions(), players_);
            }
        }
    Strand& GetStrand(){
        return api_strand_;
    }

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
        return tick_period_.has_value();
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

    void SaveState(){
        if(state_save_.has_value()){
            state_save_.value().SaveState();
        }
    }

    void LoadState(){
        if(state_save_.has_value()){
            auto game_state = state_save_.value().LoadState();
            for(const auto& [map_id, sessions] : game_state.GetAllSessions()){
                for(const auto& session_repr : sessions){
                    GameSession* session =  game_.AddSession(Map::Id(map_id));
                    /* Заполнение потерянных объектов */
                    session->SetLootObjects(session_repr.GetLoot());
                    for(const auto& dog_repr : session_repr.GetDogsRepr()){
                        /* Добавление собаки */
                        Dog* created_dog = session->AddCreatedDog(dog_repr.Restore());
                        const auto& player_repr = dog_repr.GetPlayerRepr();
                        /* Добавление игрока */
                        Player& added_player = players_.Add(player_repr.GetId(), 
                                    Player::Name(player_repr.GetName()),
                                    created_dog,
                                    session);
                        tokens_.AddPlayerWithToken(added_player, Token(player_repr.GetToken()));
                        tokens_.AddPlayerInSession(added_player, session);
                    }
                }
            }
        }
    }

    std::string IncreaseTime(unsigned delta){
        std::string res =  game_handler_.IncreaseTime(delta, game_);
        /* 
            Сохраняем игровое состояние 
            синхроннно с ходом игровых часов только в том случае, 
            когда указан файл сохранения и период
        */
        if(state_save_.has_value()){
            state_save_.value().SaveOnTick(tick_period_.has_value());
        }
        return res;
    }

    void GenerateLoot(Milliseconds delta){
        return game_handler_.GenerateLoot(delta, game_);
    }

    std::string ApplyPlayerAction(const json::object& action, const Token& token){
        return game_handler_.SetAction(action, token);
    }

    std::string GetRecords(unsigned start, unsigned max_items){
        return game_handler_.GetRecords(start, max_items);
    }
private:
    Game& game_;
    Strand api_strand_;
    std::optional<unsigned> tick_period_;
    std::optional<GameStateSaveCase> state_save_;
    bool rand_spawn_;
    Players players_;
    PlayerTokens tokens_; 
    GameUseCase game_handler_;
    std::shared_ptr<detail::Ticker> time_ticker_;
    std::shared_ptr<detail::Ticker> loot_ticker_;
};

} // namespace app