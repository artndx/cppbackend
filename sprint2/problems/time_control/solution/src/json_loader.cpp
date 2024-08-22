#include "json_loader.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>

using namespace model;

namespace json_loader {

int GetInt(std::string key, const json::object& obj){
    return static_cast<int>(obj.at(key).as_int64());
}

std::string GetString(std::string key, const json::object& obj){
    std::string result = json::serialize(obj.at(key));
    return result.substr(1, result.size() - 2);
}

void AddRoadsFromJson(const json::object& json_map, Map& map){
    json::array json_roads = json_map.at("roads").as_array();

    for(const json::value& value : json_roads){
        json::object json_road = value.as_object();

        Point point{GetInt("x0", json_road), GetInt("y0", json_road)};

        if(json_road.contains("x1")){
            map.AddRoad(Road{Road::HORIZONTAL, point, GetInt("x1", json_road)});
        } else if(json_road.contains("y1")){
            map.AddRoad(Road{Road::VERTICAL, point, GetInt("y1", json_road)});
        }
    }
}

void AddBuildingsFromJson(const json::object& json_map, Map& map){
    json::array buildings = json_map.at("buildings").as_array();

    for(const json::value& value : buildings){
        json::object json_building = value.as_object();
    
        map.AddBuilding(Building{Rectangle{
                        {GetInt("x", json_building), GetInt("y", json_building)},
                        {GetInt("w", json_building), GetInt("h", json_building)}
                        }});
    }
}

void AddOfficesFromJson(const json::object& json_map, Map& map){
    json::array offices = json_map.at("offices").as_array();
    for(const json::value& value : offices){
        json::object json_office = value.as_object();
    
        map.AddOffice(Office{
                        Office::Id{GetString("id", json_office)}, 
                        Point{GetInt("x", json_office), GetInt("y", json_office)},
                        Offset{GetInt("offsetX", json_office), GetInt("offsetY", json_office)}
                        });
    }
}

void AddMaps(const json::array& json_maps, Game& game){
    for(const json::value& value : json_maps){
        json::object json_map = value.as_object();  

        Map map{Map::Id{GetString("id", json_map)}, GetString("name", json_map)};
        double dog_speed = game.GetDefaultDogSpeed();
        try{
            if(auto it = json_map.find("dogSpeed"); it != json_map.end()){
                dog_speed = it->value().as_double();
            }
        } catch(std::exception& ex){
            std::cerr << ex.what() << std::endl;
        }
        map.AddDogSpeed(dog_speed);
        AddRoadsFromJson(json_map, map);
        AddBuildingsFromJson(json_map, map);
        AddOfficesFromJson(json_map, map);
        game.AddMap(map);
    }
}

void LoadConfig(std::string json_str, Game& game){
    try{
        json::object attributes = json::parse(json_str).as_object();

        if(auto it = attributes.find("defaultDogSpeed"); it != attributes.end()){
            game.SetDefaultDogSpeed(it->value().as_double());
        }
        if(auto it = attributes.find("maps"); it != attributes.end()){
            AddMaps(it->value().as_array(), game);
        }
    } catch(std::exception& ex){
        std::cerr << ex.what() << std::endl;
    }
}

model::Game LoadGame(const std::filesystem::path& json_path) {
    // Загрузить содержимое файла json_path, например, в виде строки
    // Распарсить строку как JSON, используя boost::json::parse
    // Загрузить модель игры из файла
    Game game;

    std::ifstream input_json(json_path);
    std::ostringstream json_stream;
    json_stream << input_json.rdbuf();
    LoadConfig(json_stream.str(), game);
    return game;
}

}  // namespace json_loader
