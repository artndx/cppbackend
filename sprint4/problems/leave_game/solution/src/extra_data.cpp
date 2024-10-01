#include "extra_data.h"

namespace Data
{
	MapExtras::MapExtras(boost::json::object config)
		:loot_config_(std::move(config)), 
		generator_(std::chrono::milliseconds{ static_cast<int>(loot_config_.at("period").as_double() * 1000) }, loot_config_.at("probability").as_double())
	{}

	void MapExtras::AddTable(const std::string& map_id, boost::json::array& table)
	{
		loot_table_by_id_[map_id] = table;
	}

	boost::json::array MapExtras::GetTable(const std::string& map_id) const
	{
		if (loot_table_by_id_.contains(map_id))
		{
			return loot_table_by_id_.at(map_id);
		}
		else
		{
			return {};
		}
	}

	boost::json::object MapExtras::GetConfig() const
	{
		return loot_config_;
	}

	loot_gen::LootGenerator* MapExtras::GetLootGenerator()
	{
		return &generator_;
	}
}