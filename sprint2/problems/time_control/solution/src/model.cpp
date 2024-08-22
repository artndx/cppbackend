#include "model.h"

#include <stdexcept>

namespace model {
using namespace std::literals;

/* ------------------------ Map ----------------------------------- */

const Map::Id& Map::GetId() const noexcept {
    return id_;
}

const std::string& Map::GetName() const noexcept {
    return name_;
}

const Map::Buildings& Map::GetBuildings() const noexcept {
    return buildings_;
}

const Map::Roads& Map::GetRoads() const noexcept {
    return roads_;
}

const Map::Offices& Map::GetOffices() const noexcept {
    return offices_;
}

void Map::AddRoad(const Road& road) {
    roads_.emplace_back(road);
    if(road.IsVertical()){
        v_roads_.insert({road.GetStart().x, &roads_.back()});
    } else{
        h_roads_.insert({road.GetStart().y, &roads_.back()});
    }
}

std::vector<const Road*> Map::FindRoadsByCoords(const Dog::Position& pos) const{
    std::vector<const Road*> result;

    FindInVerticals(pos, result);
    FindInHorizontals(pos, result);

    return result;
}

void Map::AddBuilding(const Building& building) {
    buildings_.emplace_back(building);
}

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

void Map::AddDogSpeed(double new_speed){
    dog_speed_ = new_speed;
}

double Map::GetDogSpeed() const{
    return dog_speed_;
}

void Map::FindInVerticals(const Dog::Position& pos, std::vector<const Road*>& roads) const{
    ConstRoadIt it_x = v_roads_.lower_bound((*pos).x);          /* Ищем ближайшую дорогу по полученной координате */

    if(it_x != v_roads_.end()){                 
        if(it_x != v_roads_.begin()){                   /* Если ближайшая дорога не является первой, то нужно проверить предыдущую */
            ConstRoadIt prev_it_x = std::prev(it_x, 1);
            if(CheckBounds(prev_it_x, pos)){
                roads.push_back(prev_it_x->second);
            }
        }

        if(CheckBounds(it_x, pos)){                     /* Проверяем ближайшую дорогу */
            roads.push_back(it_x->second);
        }
    } else {                                            /* Если дорога не найдена, то полученная координата дальше всех дорог*/
        it_x = std::prev(v_roads_.end(), 1);            /* Тогда проверяем последнюю дорогу */
        if(CheckBounds(it_x, pos)){
            roads.push_back(it_x->second);
        }
    }
}

void Map::FindInHorizontals(const Dog::Position& pos, std::vector<const Road*>& roads) const{
    ConstRoadIt it_y = h_roads_.lower_bound((*pos).y);          /* Ищем ближайшую дорогу по полученной координате */

    if(it_y != h_roads_.end()){                 
        if(it_y != h_roads_.begin()){                   /* Если ближайшая дорога не является первой, то нужно проверить предыдущую */
            ConstRoadIt prev_it_y = std::prev(it_y, 1);
            if(CheckBounds(prev_it_y, pos)){
                roads.push_back(prev_it_y->second);
            }
        }

        if(CheckBounds(it_y, pos)){                     /* Проверяем ближайшую дорогу */
            roads.push_back(it_y->second);
        }
    } else {                                            /* Если дорога не найдена, то полученная координата дальше всех дорог*/
        it_y = std::prev(h_roads_.end(), 1);            /* Тогда проверяем последнюю дорогу */
        if(CheckBounds(it_y, pos)){
            roads.push_back(it_y->second);
        }
    }
}

bool Map::CheckBounds(ConstRoadIt it, const Dog::Position& pos) const{
    const auto& start = it->second->GetStart();
    const auto& end = it->second->GetEnd();
    return ((start.x - 0.4 <= (*pos).x && (*pos).x <= end.x + 0.4) && 
                (start.y - 0.4 <= (*pos).y && (*pos).y <= end.y + 0.4));
}

/* ------------------------ Game ----------------------------------- */

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
        GameSession* session = &(map_id_to_sessions_[id].emplace_back(map));
        return session;
    }
    return nullptr;
}

GameSession* Game::SessionIsExists(const Map::Id& id){
    if(const Map* map = FindMap(id); map != nullptr){
        if(!map_id_to_sessions_[id].empty()){
            GameSession* session = &(map_id_to_sessions_[id].back());
            return session;
        }
    }
    return nullptr;
}

void Game::SetDefaultDogSpeed(double new_speed){
    default_dog_speed_ = new_speed;
}

double Game::GetDefaultDogSpeed() const{
    return default_dog_speed_;
}

const Game::Maps& Game::GetMaps() const noexcept {
    return maps_;
}

const Map* Game::FindMap(const Map::Id& id) const noexcept {
    if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
        return &maps_.at(it->second);
    }
    return nullptr;
}

void Game::UpdateGameState(double delta){
    for(auto& [map_id, sessions] : map_id_to_sessions_){
        for(GameSession& session : sessions){
            UpdateAllDogsPositions(session.GetDogs(), session.GetMap(), delta);
        }
    }
}

void Game::UpdateAllDogsPositions(std::deque<Dog>& dogs, const Map* map, double delta){
    for(Dog& dog : dogs){
        auto roads = map->FindRoadsByCoords(dog.GetPosition());

        const auto& [x, y] = *(dog.GetPosition());
        const auto& [vx, vy] = *(dog.GetSpeed());

        Dog::PairDouble new_pos({x + vx * delta, y + vy * delta});
        Dog::PairDouble new_speed({vx, vy});


        for(const Road* road : roads){
            Point start = road->GetStart();
            Point end = road->GetEnd();

            if(new_pos.x <= start.x - 0.4){
                new_pos.x = start.x - 0.4;
                new_speed = {0, 0};
            } else if(new_pos.x >= end.x + 0.4){
                new_pos.x = end.x + 0.4;
                new_speed = {0, 0};
            }

            if(new_pos.y <= start.y - 0.4){
                new_pos.y = start.y - 0.4;
                new_speed = {0, 0};
            } else if(new_pos.y >= end.y + 0.4){
                new_pos.y = end.y + 0.4;
                new_speed = {0, 0};
            }
            
            
        } 
        
        dog.SetPosition(Dog::Position(new_pos));
        dog.SetSpeed(Dog::Speed(new_speed));
    }
    
}


}  // namespace model