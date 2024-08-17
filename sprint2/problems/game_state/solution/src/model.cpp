#include "model.h"

#include <stdexcept>

namespace model {
using namespace std::literals;

void Map::AddOffice(Office office) {
    if (warehouse_id_to_index_.contains(office.GetId())) {
        throw std::invalid_argument("Duplicate warehouse");
    }

    const size_t index = offices_.size();
    Office& o = offices_.emplace_back(std::move(office));
    try {
        warehouse_id_to_index_.emplace(o.GetId(), index);
    } catch (...) {
        // Удаляем офис из вектора, если не удалось вставить в unordered_map
        offices_.pop_back();
        throw;
    }
}

void Game::AddMap(Map map) {
    const size_t index = maps_.size();
    if (auto [it, inserted] = map_id_to_index_.emplace(map.GetId(), index); !inserted) {
        throw std::invalid_argument("Map with id "s + *map.GetId() + " already exists"s);
    } else {
        try {
            maps_.emplace_back(std::move(map));
        } catch (...) {
            map_id_to_index_.erase(it);
            throw;
        }
    }
}

GameSession* Game::AddSession(const Map::Id& id){
    if(const Map* map = FindMap(id); map != nullptr){
        GameSession* session = &(sessions_[id].emplace_back(map));
        return session;
    }
    return nullptr;
}

GameSession* Game::SessionIsExists(const Map::Id& id){
    if(const Map* map = FindMap(id); map != nullptr){
        if(!sessions_[id].empty()){
            GameSession* session = &(sessions_[id].back());
            return session;
        }
    }
    return nullptr;
}

}  // namespace model
