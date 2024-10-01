#include "json_loader.h"
#include "tagged.h"
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace json_loader
{

	void ParseRoads(model::Map& map, json::object& parent_object)
	{
		for (auto road_entry : parent_object.at("roads").as_array())
		{
			json::object road_data = road_entry.as_object();

			if (road_data.contains(X1_COORDINATE))
			{
				model::Road road(
					{
						model::Road::HORIZONTAL, 
						{static_cast<int>(road_data.at(X0_COORDINATE).as_int64()), static_cast<int>(road_data.at(Y0_COORDINATE).as_int64())},
						static_cast<int>(road_data.at(X1_COORDINATE).as_int64())
					});

				map.AddRoad(std::move(road));
			}
			else
			{
				model::Road road(
					{
						model::Road::VERTICAL,
						{static_cast<int>(road_data.at(X0_COORDINATE).as_int64()), static_cast<int>(road_data.at(Y0_COORDINATE).as_int64())},
						static_cast<int>(road_data.at(Y1_COORDINATE).as_int64())
					});

				map.AddRoad(std::move(road));
			}

		}
		map.CalcRoads();
	}


	void ParseBuildings(model::Map& map, json::object& parent_object)
	{
		for (auto building_entry : parent_object.at("buildings").as_array())
		{
			json::object building_data = building_entry.as_object();

			model::Building building(
				{
					{static_cast<int>(building_data.at(X_COORDINATE).as_int64()), static_cast<int>(building_data.at(Y_COORDINATE).as_int64())},
					{static_cast<int>(building_data.at(WIDTH).as_int64()), static_cast<int>(building_data.at(HEIGHT).as_int64())}
				});

			map.AddBuilding(std::move(building));
		}
	}

	void ParseOffices(model::Map& map, json::object& parent_object)
	{
		for (auto office_entry : parent_object.at("offices").as_array())
		{
			json::object office_data = office_entry.as_object();

			using Id = util::Tagged<std::string, model::Office>;
			Id id{ std::string(office_data.at("id").as_string()) };

			model::Office office(
				{
					id,
					{static_cast<int>(office_data.at(X_COORDINATE).as_int64()), static_cast<int>(office_data.at(Y_COORDINATE).as_int64())},
					{static_cast<int>(office_data.at(X_OFFSET).as_int64()), static_cast<int>(office_data.at(Y_OFFSET).as_int64())}
				});

			map.AddOffice(std::move(office));
		}
	}

	model::Game LoadGame(const std::filesystem::path& json_path, model::Players& pm, db::ConnectionPool& connection_pool)
	{
		// Загрузить содержимое файла json_path, например, в виде строки
		// Распарсить строку как JSON, используя boost::json::parse
		// Загрузить модель игры из файла
		model::Game game{pm, connection_pool };

		std::ifstream file(json_path);

		if (!file.is_open())
		{
			json::object tmp{ {"error loading file", json_path.string()} };
			json::object logger_data{ {"code", "fileLoadingError"}, {"exception", tmp} };

			BOOST_LOG_TRIVIAL(info) << logging::add_value(timestamp, pt::microsec_clock::local_time()) << logging::add_value(additional_data, logger_data) << "error";
			throw std::invalid_argument("Loading Error! Game data file couldn't be opened..");
		}

		std::ostringstream tmp;

		tmp << file.rdbuf();

		std::string json_str = tmp.str();

		auto value = json::parse(json_str);

		if (value.as_object().contains("defaultDogSpeed"))
		{
			game.SetGlobalDogSpeed(value.as_object().at("defaultDogSpeed").as_double());
		}

		if (value.as_object().contains("defaultBagCapacity"))
		{
			game.SetGlobalCapacity(value.as_object().at("defaultBagCapacity").as_int64());
		}

		if (value.as_object().contains("dogRetirementTime"))
		{
			game.SetAFK(value.as_object().at("dogRetirementTime").as_double());
		}

		if (value.as_object().contains("lootGeneratorConfig"))
		{
			Data::MapExtras extras{ value.as_object().at("lootGeneratorConfig").as_object() };
			game.SetExtraData(extras);
		}

		json::array maps = value.as_object().at("maps").as_array();

		for (auto entry : maps)
		{
			//Unpacking JSON object that contains data for the current map
			json::object map_data = entry.as_object();


			using Id = util::Tagged<std::string, model::Map>;
			Id id{ std::string(map_data.at("id").as_string()) };

			//Initializing map
			model::Map map(id, std::string(map_data.at("name").as_string()), game.GetPool()); //So many brackets omg

			double dog_speed = -1;
			int bag_capacity =-1;

			if (map_data.contains("dogSpeed"))
			{
				dog_speed = map_data.at("dogSpeed").as_double();
			}

			if (map_data.contains("bagCapacity"))
			{
				bag_capacity = map_data.at("bagCapacity").as_int64();
			}

			if (map_data.contains("lootTypes"))
			{
				game.AddTable(*id, map_data.at("lootTypes").as_array());
			}

			//Reading JSON-object containing roads from map_data and adding them to the map
			ParseRoads(map, map_data);

			//Doing the same with buildings and offices
			ParseBuildings(map, map_data);
			ParseOffices(map, map_data);

			//Adding the map to the game
			game.AddMap(map, dog_speed, bag_capacity);

		}

		return game;
	}

}  // namespace json_loader
