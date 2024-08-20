#include "app.h"
#include <stdexcept>
#include <iostream>


namespace app{

/* ------------------------ Players ----------------------------------- */

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

/* ---------------------- PlayerTokens ------------------------------------- */

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

Token PlayerTokens::GenerateToken() {
    std::ostringstream out;
    out << std::setw(32) << std::setfill('0') << std::hex << generator1_() << generator2_();
    std::string token = out.str();
    size_t size = token.size();
    if(token.size() > 32){
        token = token.substr(0, 32);
    }
    return Token(token);
}

/* ------------------------ GetMapUseCase ----------------------------------- */

std::string GetMapUseCase::MakeMapDescription(const model::Map* map){
    json::object map_description;

    map_description["id"] = *(map->GetId());
    map_description["name"] =  map->GetName();
    map_description["roads"] = GetRoadsInJSON(map->GetRoads());
    map_description["buildings"] = GetBuildingsInJSON(map->GetBuildings());
    map_description["offices"] = GetOfficesInJSON(map->GetOffices());

    return json::serialize(map_description); 
}


json::array GetMapUseCase::GetRoadsInJSON(const model::Map::Roads& roads){
    json::array result;

    for(const model::Road& road : roads){
        json::object obj;
        obj["x0"] = road.GetStart().x;
        obj["y0"] = road.GetStart().y;
        if(road.IsHorizontal()){
            obj["x1"] = road.GetEnd().x;
        } else if(road.IsVertical()){
            obj["y1"] = road.GetEnd().y;
        } else {
            break;
        }

        result.push_back(obj);
    }

    return result;
}

json::array GetMapUseCase::GetBuildingsInJSON(const model::Map::Buildings& buildings){
    json::array result;

    for(const model::Building& building : buildings){
        json::object obj;
        obj["x"] = building.GetBounds().position.x;
        obj["y"] = building.GetBounds().position.y;
        obj["w"] = building.GetBounds().size.width;
        obj["h"] = building.GetBounds().size.height;
        result.push_back(obj);
    }

    return result;
}

json::array GetMapUseCase::GetOfficesInJSON(const model::Map::Offices& offices){
    json::array result;

    for(const model::Office& office : offices){
        json::object obj;
        obj["id"] = *(office.GetId());
        obj["x"] = office.GetPosition().x;
        obj["y"] = office.GetPosition().y;
        obj["offsetX"] = office.GetOffset().dx;
        obj["offsetY"] = office.GetOffset().dy;
        result.push_back(obj);
    }

    return result;
}

/* ------------------------ ListMapsUseCase ----------------------------------- */

std::string ListMapsUseCase::MakeMapsList(const model::Game::Maps& maps){
    json::array map_list;
    for(const model::Map& map : maps){
        json::object obj;
        obj["id"] = *(map.GetId());
        obj["name"] = map.GetName();
        map_list.push_back(obj);
    }

    return json::serialize(map_list);
}

/* ------------------------ GameUseCase ----------------------------------- */

std::string GameUseCase::JoinGame(const std::string& user_name, const std::string& str_map_id, model::Game& game){
    using namespace std::literals;
    model::Map::Id map_id(str_map_id);

    model::GameSession* session = game.SessionIsExists(map_id);
    if(session == nullptr){
        session = game.AddSession(map_id);
    }
    const model::Dog* dog = session->AddDog(auto_counter_, model::Dog::Name(user_name),
                                            model::Dog::Position(GetRandomPos(game.FindMap(map_id)->GetRoads())), 
                                            model::Dog::Speed({0,0}),
                                            model::Direction::NORTH);
    const app::Player& player = players_.Add(auto_counter_, app::Player::Name(user_name), dog, session);
    ++auto_counter_;

    app::Token token = tokens_.AddPlayer(player);
    
    json::object json_body;
    json_body["authToken"] = *token;
    json_body["playerId"] = player.GetId();

    return json::serialize(json_body);   
}

std::pair<double, double> GameUseCase::GetRandomPos(const model::Map::Roads& roads) const{
    auto random_num = [](int a, int b){
        return a + rand()%(b-a);
    };

    size_t road_index = random_num(0, roads.size());
    const model::Road& road = roads[road_index];
    double x = 0;
    double y = 0;
    if(road.IsHorizontal()){
        x = random_num(road.GetStart().x, road.GetEnd().x);
        y = road.GetStart().y;
    } else if(road.IsVertical()){
        x = road.GetStart().x;
        y = random_num(road.GetStart().y, road.GetEnd().y);
    }
    return {x,y};
}

std::string GameUseCase::GetGameState() const{
    json::object players;
    
    for(const auto& [key, player] : players_.GetPlayers()){
        json::object player_attributes;

        std::pair<double, double> pos = *(player.GetDog()->GetPosition());
        player_attributes["pos"] = {pos.first, pos.second};
        
        std::pair<double, double> speed = *(player.GetDog()->GetSpeed());
        player_attributes["speed"] = {speed.first, speed.second};

        model::Direction dir = player.GetDog()->GetDirection();
        switch (dir)
        {
            case model::Direction::NORTH:
                player_attributes["dir"] = "U";
                break;
            case model::Direction::SOUTH:
                player_attributes["dir"] = "D";
                break;
            case model::Direction::WEST:
                player_attributes["dir"] = "L";
                break;
            case model::Direction::EAST:
                player_attributes["dir"] = "R";
                break;
            default:
                player_attributes["dir"] = "Unknown";
        }

        players[std::to_string(key.first)] = player_attributes;
    }

    json::object result;
    result["players"] = players;
    return json::serialize(result);
}

std::string GameUseCase::SetAction(const json::object& action, const Token& token){
    std::string dir = std::string(action.at("move").as_string());
    if(dir == "U"){

    } else if(dir == "D"){
        
    } else if(dir == "L"){
        
    } else if(dir == "R"){
        
    } else if(dir == ""){
        
    }
    return "{}";
}


/* ------------------------ ListPlayersUseCase ----------------------------------- */

std::string ListPlayersUseCase::GetPlayersInJSON(const app::Players::PlayerList& players){
    json::object player_list;
    for(const auto& [key, player ] : players){
        json::object player_description;
        player_description["name"] = *(player.GetName());
        player_list[std::to_string(player.GetId())] = player_description;
    }
    return json::serialize(player_list);
}
}; //namespace app