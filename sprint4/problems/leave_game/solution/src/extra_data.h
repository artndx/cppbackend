#pragma once

#include <boost/json.hpp>
#include "loot_generator.h"

namespace json = boost::json;

namespace Data
{
	class MapExtras
	{
		public:

			MapExtras() = default;

			explicit MapExtras(boost::json::object config);

			void AddTable(const std::string& map_id, boost::json::array& table);

			boost::json::array GetTable(const std::string& map_id) const;

			boost::json::object GetConfig() const;

			loot_gen::LootGenerator* GetLootGenerator();

		private:

			boost::json::object loot_config_;
			std::unordered_map<std::string, boost::json::array> loot_table_by_id_;

			loot_gen::LootGenerator generator_{ std::chrono::milliseconds{5}, 1 };
	};
}