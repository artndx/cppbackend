#include <random>
#include "model.h"

namespace util {

using DogMapKey = std::pair<int, model::Map::Id>;
struct DogMapKeyHasher{
    size_t operator()(const DogMapKey& value) const;
};

} // namespace util

namespace model{

namespace detail {
struct TokenTag {};
}  // namespace detail


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
    using PlayerList = std::unordered_map<util::DogMapKey, Player, util::DogMapKeyHasher>;
    Players() = default;

    Player& Add(int id, const Player::Name& name, Dog* dog, const GameSession* session);

    const Player* FindByDogIdAndMapId(int dog_id, std::string map_id) const;

    const PlayerList& GetPlayers() const;
private:
    PlayerList players_;
};

/* ---------------------- PlayerTokens ------------------------------------- */

using Token = util::Tagged<std::string, detail::TokenTag>;

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

} // namespace model