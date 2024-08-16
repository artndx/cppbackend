#include "api.h"
#include <stdexcept>


namespace api{

size_t Players::DogMapKeyHasher::operator()(const DogMapKey& value) const{
    size_t h1 = static_cast<size_t>(value.first);
    size_t h2 = util::TaggedHasher<Map::Id>()(value.second);

    return h1 * 37 + h2 * 37 * 37;
}

Player& Players::Add(int id, const Player::Name& name, const Dog* dog, const GameSession* session){
    DogMapKey key = std::make_pair(dog->GetId(), session->GetMap()->GetId());
    Player player(id, name, dog, session);
    auto [it, is_emplaced] = players_.emplace(key, player);
    if(is_emplaced){
        return it->second;
    }

    throw std::logic_error("Player has already been added");
}

const Player* Players::FindByDogIdAndMapId(int dog_id, std::string map_id) const{
    try{
        return &players_.at(DogMapKey(std::make_pair(dog_id, map_id)));
    } catch(...){
        return nullptr;
    }
}

const Players::PlayerList& Players::GetPlayers() const{
    return players_;
}

Token PlayerTokens::AddPlayer(const Player& player){
    auto [it, is_emplaced] = token_to_player_.emplace(GenerateToken(), &player);
    if(is_emplaced){
        return it->first;
    }

    throw std::logic_error("Player with this token has already been added");
}

const Player* PlayerTokens::FindPlayerByToken(const Token& token) const{
    try{
        return token_to_player_.at(token);
    } catch(...){
        return nullptr;
    }
}

}; //namespace api