#include "model_core.h"

int GetRandomNumber(int range)
{
	std::srand(static_cast<unsigned int>(std::chrono::system_clock::now().time_since_epoch().count()));

	return rand() % range;
}

std::string GenerateToken()
{

	const char hexChar[] =
	{
		'0', '1', '2', '3', '4',
		'5', '6', '7', '8', '9',
		'a','b','c','d','e','f'
	};

	std::string token;

	for (int i = 0; i < 32; i++)
	{
		token += hexChar[GetRandomNumber(16)];
	}

	return token;
}

namespace model
{
	using namespace std::literals;

	//===Map===	
	void Map::AddOffice(Office office)
	{

		if (warehouse_id_to_index_.contains(office.GetId()))
		{
			throw std::invalid_argument("Duplicate warehouse");
		}

		const size_t index = offices_.size();
		Office& o = offices_.emplace_back(std::move(office));
		try {
			warehouse_id_to_index_.emplace(o.GetId(), index);
		}
		catch (...) {
			// Удаляем офис из вектора, если не удалось вставить в unordered_map
			offices_.pop_back();
			throw;
		}
	}
	void Map::CalcRoads()
	{
		for (Road& road : roads_)
		{
			Point start = road.GetStart();
			Point end = road.GetEnd();

			int small, big;

			if (road.IsHorizontal())
			{
				int x1 = start.x, x2 = end.x;

				if (x1 > x2)
				{
					small = x2;
					big = x1;
				}
				else
				{
					big = x2;
					small = x1;
				}

				for (int i = small; i <= big; ++i)
				{
					point_to_roads_[{i, start.y}].push_back(std::make_shared<Road>(road));
				}

			}
			else
			{
				int y1 = start.y, y2 = end.y;

				if (y1 > y2)
				{
					small = y2;
					big = y1;
				}
				else
				{
					big = y2;
					small = y1;
				}

				for (int i = small; i <= big; ++i)
				{
					point_to_roads_[{start.x, i}].push_back(std::make_shared<Road>(road));
				}
			}
		}
	}

	void Map::GenerateItems(unsigned int amount, const Data::MapExtras& extras)
	{
		json::array loot_table_ = extras.GetTable(*id_);

		size_t s = items_.size();

		for (int i = 0; i < amount; ++i)
		{
			int id = s + i;
			int item_type_id = GetRandomNumber(loot_table_.size());
			Coordinates pos = GetRandomSpot();
			int64_t value = loot_table_[item_type_id].as_object().at("value").as_int64();

			items_.push_back({ pos, id, item_type_id, value });

			json::object logger_data;
			logger_data.emplace("ID", id);
			logger_data.emplace("Type", item_type_id);
			logger_data.emplace("Value", value);
			logger_data.emplace("Position X", pos.x);
			logger_data.emplace("Position Y", pos.y);

			BOOST_LOG_TRIVIAL(info) << logging::add_value(timestamp, pt::microsec_clock::local_time()) << logging::add_value(additional_data, logger_data) << "generated item";
		}
	}

	Coordinates Map::GetRandomSpot() const
	{
		//Getting a random road
		const std::vector<Road>& roads = GetRoads();
		const Road& road = roads.at(GetRandomNumber(roads.size()));

		//Now we find the precise spot on that road
		Coordinates spot;

		Point start = road.GetStart();
		Point end = road.GetEnd();

		if (road.IsVertical())
		{
			spot.x = start.x;

			int offset = GetRandomNumber(std::abs(start.y - end.y));

			if (start.y < end.y)
			{
				spot.y = start.y + offset;
			}
			else
			{
				spot.y = end.y + offset;
			}
		}
		else
		{
			spot.y = start.y;

			int offset = GetRandomNumber(std::abs(start.x - end.x));

			if (start.x < end.x)
			{
				spot.x = start.x + offset;
			}
			else
			{
				spot.x = end.x + offset;
			}
		}

		return spot;
	}

	bool operator==(const Road& r1, const Road& r2)
	{
		return r1.start_ == r2.start_ && r1.end_ == r2.end_;
	}

	const Road& Map::FindRoad(Point start, Point end) const
	{
		auto iter = std::find(roads_.begin(), roads_.end(), start.x == end.x ? Road(Road::VERTICAL, start, end.y) : Road(Road::HORIZONTAL, start, end.x));

		return *iter;
	}

	void Map::RetireDog(const std::string& username, int64_t score, int64_t time_alive) const
	{
		db::ConnectionPool::ConnectionWrapper wrap = connection_pool_.GetConnection();

		pqxx::work work{ *wrap };
		work.exec_params(R"(INSERT INTO retired_players (id, name, score, play_time_ms) VALUES ($1, $2, $3, $4) ON CONFLICT (id) DO UPDATE SET name=$2, score=$3, play_time_ms=$4;)"_zv,
			util::TaggedUUID<Id>::New().ToString(), username, score, time_alive);
		work.commit();
	}

} //~namespace model