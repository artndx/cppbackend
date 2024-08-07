#include "request_handler.h"
#include <boost/json.hpp>

namespace json = boost::json;

namespace http_handler {

namespace json_responses{

json::array GetRoadsFromJSON(const model::Map::Roads& roads){
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

json::array GetBuildingsFromJSON(const model::Map::Buildings& buildings){
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

json::array GetOfficesFromJSON(const model::Map::Offices& offices){
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
    map_description["roads"] = GetRoadsFromJSON(map->GetRoads());
    map_description["buildings"] = GetBuildingsFromJSON(map->GetBuildings());
    map_description["offices"] = GetOfficesFromJSON(map->GetOffices());

    return json::serialize(map_description);
}

std::string MakeMapNotFound(){
    json::object body;
    body["code"] = "mapNotFound";
    body["message"] = "Map not found";
    return json::serialize(body);
}

std::string MakeBadRequest(){
    json::object body;
    body["code"] = "badRequest";
    body["message"] = "Bad request";
    return json::serialize(body);
}

} // namespace json_responses

}  // namespace http_handler
