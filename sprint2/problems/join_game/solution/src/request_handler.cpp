#include "request_handler.h"

namespace request_handler {

namespace json_responses{

json::array GetRoadsInJSON(const model::Map::Roads& roads){
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

json::array GetBuildingsInJSON(const model::Map::Buildings& buildings){
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

json::array GetOfficesInJSON(const model::Map::Offices& offices){
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

json::object GetPlayersInJSON(const api::Players::PlayerList& players){
    json::object player_list;
    for(const auto& [key, player ] : players){
        json::object player_description;
        player_description["name"] = *(player.GetName());
        player_list[std::to_string(player.GetId())] = player_description;
    }
    return player_list;
}

std::string MakeMapsList(const model::Game::Maps& maps){
    json::array map_list;
    for(const model::Map& map : maps){
        json::object obj;
        obj["id"] = *(map.GetId());
        obj["name"] = map.GetName();
        map_list.push_back(obj);
    }

    return json::serialize(map_list);
}

std::string MakeMapDescription(const model::Map* map){
    json::object map_description;
    map_description["id"] = *(map->GetId());
    map_description["name"] =  map->GetName();
    map_description["roads"] = GetRoadsInJSON(map->GetRoads());
    map_description["buildings"] = GetBuildingsInJSON(map->GetBuildings());
    map_description["offices"] = GetOfficesInJSON(map->GetOffices());

    return json::serialize(map_description);
}

std::string MakeMapNotFound(){
    json::object body;
    body["code"] = "mapNotFound";
    body["message"] = "Map not found";
    return json::serialize(body);
}

std::string MakeErrorCode(std::string_view code, std::string_view message){
    json::object body;
    body["code"] = std::string(code);
    body["message"] = std::string(message);
    return json::serialize(body);
}

} // namespace json_responses

}  // namespace request_handler
