#include <iomanip>
#include "player.h"

namespace util {

size_t DogMapKeyHasher::operator()(const DogMapKey& value) const{
    size_t h1 = static_cast<size_t>(value.first);
    size_t h2 = util::TaggedHasher<model::Map::Id>()(value.second);

    return h1 * 37 + h2 * 37 * 37;
}

} // namespace util

namespace model{

/* ------------------------ Players ----------------------------------- */

Player& Players::Add(int id, const Player::Name& name, Dog* dog, const GameSession* session){
    util::DogMapKey key = std::make_pair(dog->GetId(), session->GetMap()->GetId());
    Player player(id, name, dog, session);
    auto [it, is_emplaced] = players_.emplace(key, player);
    if(is_emplaced){
        return it->second;
    }

    throw std::logic_error("Player has already been added");
}

const Player* Players::FindByDogIdAndMapId(int dog_id, std::string map_id) const{
    if(players_.contains(util::DogMapKey(std::make_pair(dog_id, map_id)))){
        return &players_.at(util::DogMapKey(std::make_pair(dog_id, map_id)));
    } else {
        return nullptr;
    }
}

const Players::PlayerList& Players::GetPlayers() const{
    return players_;
}

/* ---------------------- PlayerTokens ------------------------------------- */

Token PlayerTokens::AddPlayer(Player& player){
    auto [it, is_emplaced] = token_to_player_.emplace(GenerateToken(), &player);
    if(is_emplaced){
        return it->first;
    }

    throw std::logic_error("Player with this token has already been added");
}

Player* PlayerTokens::FindPlayerByToken(const Token& token){
    if(token_to_player_.contains(token)){
        return token_to_player_.at(token);
    } 
    return nullptr;
}

const Player* PlayerTokens::FindPlayerByToken(const Token& token) const{
    if(token_to_player_.contains(token)){
        return const_cast<const Player*>(token_to_player_.at(token));
    } 
    return nullptr;
}

Token PlayerTokens::GenerateToken() {
    std::ostringstream out;
    out << std::hex << std::setw(16) << std::setfill('0') << generator1_();
    out << std::hex << std::setw(16) << std::setfill('0') << generator2_();
    return Token(out.str());
}

} // namespace model