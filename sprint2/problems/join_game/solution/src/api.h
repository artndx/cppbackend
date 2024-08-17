#include "model.h"
#include <random>
#include <sstream>
#include <iomanip>

namespace api{

using namespace model;

namespace detail {
struct TokenTag {};
}  // namespace detail

using Token = util::Tagged<std::string, detail::TokenTag>;


class Players;
class PlayerTokens;

/* ----------------------------------------------------------- */

class Player{
public:
    using Name = util::Tagged<std::string, Player>;
    int GetId() const{
        return id_;
    }

    Name GetName() const{
        return name_;
    }
private:
    friend PlayerTokens;
    friend Players;

    Player(int id, Name name, const Dog* dog, const GameSession* session)
        : id_(id), name_(name), dog_(dog), session_(session){
    }

    int id_;
    Name name_;
    const Dog* dog_;
    const GameSession* session_;
};

/* ----------------------------------------------------------- */

class Players{
public:

    using DogMapKey = std::pair<int, Map::Id>;
    struct DogMapKeyHasher{
        size_t operator()(const DogMapKey& value) const;
    };
    using PlayerList = std::unordered_map<DogMapKey, Player, DogMapKeyHasher>;
    Players() = default;

    Player& Add(int id, const Player::Name& name, const Dog* dog, const GameSession* session);

    const Player* FindByDogIdAndMapId(int dog_id, std::string map_id) const;

    const PlayerList& GetPlayers() const;
private:
    PlayerList players_;
};

/* ----------------------------------------------------------- */

class PlayerTokens{
public:
    PlayerTokens() = default;

    Token AddPlayer(const Player& player);

    const Player* FindPlayerByToken(const Token& token) const;
private:
    Token GenerateToken(){
        std::ostringstream out;
        out << std::setw(32) << std::setfill('0') << std::hex << generator1_() << generator2_();
        std::string token = out.str();
        size_t size = token.size();
        if(token.size() > 32){
            token = token.substr(0, 32);
        }
        return Token(token);
    }
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
    std::unordered_map<Token, const Player *, util::TaggedHasher<Token>> token_to_player_;
};

}; // namespace api