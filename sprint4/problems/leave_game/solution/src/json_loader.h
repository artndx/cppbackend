#pragma once

#include <filesystem>
#include <string>
#include <boost/json.hpp>
#include "http_server.h"
#include "model.h"
#include "extra_data.h"

namespace json_loader 
{
	namespace json = boost::json;

	void ParseRoads(model::Map& map, json::object& parent_object);
	void ParseBuildings(model::Map& map, json::object& parent_object);
	void ParseOffices(model::Map& map, json::object& parent_object);

	model::Game LoadGame(const std::filesystem::path& json_path, model::Players& pm, db::ConnectionPool& connection_pool);

}  // namespace json_loader
