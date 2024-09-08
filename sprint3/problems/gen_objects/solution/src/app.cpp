#include "app.h"
#include <stdexcept>
#include <iostream>


namespace app{

/* ------------------------ GetMapUseCase ----------------------------------- */

std::string GetMapUseCase::MakeMapDescription(const model::Map* map){
    json::object map_description;

    map_description["id"] = *(map->GetId());
    map_description["name"] =  map->GetName();
    map_description["roads"] = GetRoadsInJSON(map->GetRoads());
    map_description["buildings"] = GetBuildingsInJSON(map->GetBuildings());
    map_description["offices"] = GetOfficesInJSON(map->GetOffices());
    map_description["lootTypes"] = GetLootTypesInJSON(map->GetLootTypes());

    return json::serialize(map_description); 
}


json::array GetMapUseCase::GetRoadsInJSON(const model::Map::Roads& roads){
    json::array result;

    for(const Road& road : roads){
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

json::array GetMapUseCase::GetLootTypesInJSON(const model::Map::LootTypes& loot_types){
    json::array result;

    for(const model::LootType& lt : loot_types){
        json::object obj;
        if(lt.name.has_value()){
            obj["name"] = *lt.name;
        }
        if(lt.file.has_value()){
            obj["file"] = *lt.file;
        }
        if(lt.type.has_value()){
            obj["type"] = *lt.type;
        }
        if(lt.rotation.has_value()){
            obj["rotation"] = *lt.rotation;
        }
        if(lt.color.has_value()){
            obj["color"] = *lt.color;
        }
        if(lt.scale.has_value()){
            obj["scale"] = *lt.scale;
        }

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

std::string GameUseCase::JoinGame(const std::string& user_name, const std::string& str_map_id, 
                        model::Game& game, bool random_spawn){
    using namespace std::literals;
    model::Map::Id map_id(str_map_id);

    model::GameSession* session = game.SessionIsExists(map_id);
    if(session == nullptr){
        session = game.AddSession(map_id);
    }

    model::Dog::Name dog_name(user_name);
    model::Dog::Position dog_pos = (random_spawn) 
        ? model::Dog::Position(model::detail::GetRandomPos(game.FindMap(map_id)->GetRoads())) 
        : model::Dog::Position(model::detail::GetFirstPos(game.FindMap(map_id)->GetRoads()));
    model::Dog::Speed dog_speed({0, 0});
    model::Direction dog_dir = model::Direction::NORTH;

    model::Dog* dog = session->AddDog(auto_counter_, dog_name, dog_pos, 
                                        dog_speed, dog_dir);

    session->UpdateLoot(session->GetDogs().size() - session->GetLootObjects().size());
    model::Player& player = players_.Add(auto_counter_, model::Player::Name(user_name), 
                                        dog, session);
    ++auto_counter_;

    model::Token token = tokens_.AddPlayer(player);
    
    json::object json_body;
    json_body["authToken"] = *token;
    json_body["playerId"] = player.GetId();

    return json::serialize(json_body);   
}

json::object GameUseCase::GetPlayers(const model::GameSession* session) const{
    json::object players;

    for(const Player* player : tokens_.GetPlayersBySession(session)){
        json::object player_attributes;

        const detail::PairDouble& pos = *(player->GetDog()->GetPosition());
        player_attributes["pos"] = {pos.x, pos.y};
        
        const detail::PairDouble& speed = *(player->GetDog()->GetSpeed());
        player_attributes["speed"] = {speed.x, speed.y};

        model::Direction dir = player->GetDog()->GetDirection();
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

        players[std::to_string(player->GetId())] = player_attributes;
    }

    return players;
}

json::object GameUseCase::GetLostObjects(const model::GameSession* session) const{
    json::object lost_objects;
    
    for(const model::Loot& loot : session->GetLootObjects()){
        json::object loot_decs;

        loot_decs["type"] = loot.type;
        json::array pos = { loot.pos.x, loot.pos.y };
        loot_decs["pos"] = pos;

        lost_objects[loot.id] = loot_decs;
    }

    return lost_objects;
}

std::string GameUseCase::GetGameState(const model::Token& token) const{
    json::object result;
    const model::GameSession* session = tokens_.FindPlayerByToken(token)->GetSession();

    result["players"] = GetPlayers(session);
    result["lostObjects"] = GetLostObjects(session);

    return json::serialize(result);
}

std::string GameUseCase::SetAction(const json::object& action, const model::Token& token){
    model::Player* player = tokens_.FindPlayerByToken(token);
    double dog_speed = player->GetSession()->GetMap()->GetDogSpeed();
    model::Direction new_dir;
    model::Dog::Speed new_speed({0, 0});    
    std::string dir = std::string(action.at("move").as_string());
    if(dir == "U"){
        new_speed = model::Dog::Speed({0, -dog_speed});
        new_dir = model::Direction::NORTH;
    } else if(dir == "D"){
        new_speed = model::Dog::Speed({0, dog_speed});
        new_dir = model::Direction::SOUTH;
    } else if(dir == "L"){
        new_speed = model::Dog::Speed({-dog_speed, 0});
        new_dir = model::Direction::WEST;
    } else if(dir == "R"){
        new_speed = model::Dog::Speed({dog_speed, 0});
        new_dir = model::Direction::EAST;
    }
    player->GetDog()->SetSpeed(new_speed);
    player->GetDog()->SetDirection(new_dir);
    return "{}";
}

std::string GameUseCase::IncreaseTime(double delta, model::Game& game){
    game.UpdateGameState(delta);
    return "{}";
}

void GameUseCase::GenerateLoot(detail::Milliseconds delta, model::Game& game){
    game.GenerateLootInSessions(delta);
}

/* ------------------------ ListPlayersUseCase ----------------------------------- */

std::string ListPlayersUseCase::GetPlayersInJSON(const model::PlayerTokens::PlayersInSession& players){
    json::object player_list;
    for(const Player* player : players){
        json::object player_description;
        player_description["name"] = *(player->GetName());
        player_list[std::to_string(player->GetId())] = player_description;
    }
    return json::serialize(player_list);
}
}; //namespace app