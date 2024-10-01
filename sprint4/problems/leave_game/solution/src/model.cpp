#include "model.h"

namespace model
{
	using namespace std::literals;

	//===Game===
	void Game::AddMap(Map map, double dog_speed, int bag_capacity) 
	{
		if (dog_speed == -1)
		{
			map.SetDogSpeed(global_dog_speed_);
		}
		else
		{
			map.SetDogSpeed(dog_speed);
		}

		if (bag_capacity == -1)
		{
			map.SetBagCapacity(global_bag_capacity);
		}
		else
		{
			map.SetBagCapacity(bag_capacity);
		}

		map.SetAFK(afk_threshold);

		const size_t index = maps_.size();
		if (auto [it, inserted] = map_id_to_index_.emplace(map.GetId(), index); !inserted) {
			throw std::invalid_argument("Map with id "s + *map.GetId() + " already exists"s);
		}
		else 
		{
			try 
			{
				maps_.emplace_back(std::move(map));
			}
			catch (...) 
			{
				map_id_to_index_.erase(it);
				throw;
			}
		}
	}

	void Game::MoveAndCalcPickups(Map& map, int ms)
	{
		std::string map_id{ *(map.GetId()) };

		//Saving positions of players with an empty slots in their bags before moving them
		std::deque<Coordinates> start_positions{ player_manager_.GetLooterPositionsByMap(map_id) };

		//Moving Players
		player_manager_.MoveAllByMap(ms, map_id);

		//Saving positions of players with an empty slots in their bags
		std::deque<Coordinates> end_positions{ player_manager_.GetLooterPositionsByMap(map_id) };

		//Getting a list of all items on the map
		const std::deque<Item>& map_items = map.GetItemList();

		//Temporary container to feed ItemProvider (Item Data)
		std::vector<collision_detector::Item> items;

		for (const auto& item : map_items)
		{
			items.push_back({ {item.pos.x, item.pos.y}, item.width });
		}


		//Temporary container to feed ItemProvider (Gatherer Data)
		std::vector<collision_detector::Gatherer> gatherers;

		for (int i = 0; i < start_positions.size(); ++i)
		{
			gatherers.push_back({ {start_positions[i].x, start_positions[i].y}, {end_positions[i].x, end_positions[i].y}, .3 });
		}

		//The provider itself
		collision_detector::VectorItemGathererProvider provider{ items, gatherers };

		//Calculating collisions
		auto events = collision_detector::FindGatherEvents(provider);

		//Items to remove
		std::set<int> removed_ids;

		for (collision_detector::GatheringEvent& loot_event : events)
		{
			int item_id = loot_event.item_id;
			removed_ids.insert(item_id);

			player_manager_.FindPlayerByIdx(loot_event.gatherer_id)->StoreItem(map.GetItemByIdx(item_id));
		}

		for (auto iter = removed_ids.rbegin(); iter != removed_ids.rend(); ++iter)
		{
			map.RemoveItem(*iter);
		} //Wonder if I forgot anything..

		for(auto& player : player_manager_.GetPlayerList(map_id))
		{
			Coordinates player_pos = player->GetPos();

			for (auto& office : map.GetOffices())
			{
				Point office_pos_int = office.GetPosition();

				double w = std::abs(player_pos.x - office_pos_int.x);
				double h = std::abs(player_pos.y - office_pos_int.y);

				double res = std::sqrt(std::pow(w, 2) + std::pow(h, 2));

				if (res <= 0.55)
				{
					player->Depot();
					break;
				}
			}
		}
	}

	void Game::ServerTick(int milliseconds)
	{
		loot_gen::LootGenerator* gen_ptr = extra_data_.GetLootGenerator();

		if (gen_ptr != nullptr)
		{
			for (Map& map : maps_)
			{
				MoveAndCalcPickups(map, milliseconds);

				unsigned player_count = GetPlayerCount(*map.GetId());
				int item_count = map.GetItemCount();

				map.GenerateItems(gen_ptr->Generate(std::chrono::milliseconds{ milliseconds }, item_count, player_count), extra_data_);
			}
		}
	}

	void Game::SetLootOnMap(const std::deque<Item>& items, const std::string& map_id)
	{
		for (Map& map : maps_)
		{
			if (*map.GetId() == map_id)
			{
				map.SetItems(items);
			}
		}
	}

	const Map* Game::FindMap(const Map::Id& id) const noexcept
	{
		if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end())
		{
			return &maps_.at(it->second);
		}
		return nullptr;
	}

	//===Dog===
	Dog::Dog(Coordinates coords, const Map* map)
		:position_(coords)
		, speed_(map->GetDogSpeed())
		, current_map_(map)
	{
		const std::deque<std::shared_ptr<Road>>& roads = GetRoadsByPos(coords);
		current_road_ = roads.front().get();

		Point start = current_road_->GetStart();
		Point end = roads[0]->GetEnd();
	}


	Dog::Dog(Coordinates coords, Road* current_road, Velocity vel, double speed, Direction dir, const Map* map)
	:position_(coords),
	current_road_(current_road),
	velocity_(vel),
	speed_(speed),
	direction_(dir),
	current_map_(map){}

	void Dog::MoveVertical(double distance, int iteration)
	{
		if (distance == 0 || iteration > 2)
			return;

		Point start = current_road_->GetStart();
		Point end = current_road_->GetEnd();

		double desired_point = position_.y + distance;

		double small_y, big_y;

		if (start.y < end.y)
		{
			small_y = start.y;
			big_y = end.y;
		}
		else
		{
			small_y = end.y;
			big_y = start.y;
		}

		double y1 = small_y - 0.4;
		double y2 = big_y + 0.4;

		if (desired_point < y1 || desired_point > y2)
		{
			for (std::shared_ptr<Road> road : GetRoadsByPos(position_))
			{
				if (road != nullptr)
				{
					if (road.get() != current_road_ && road.get()->IsVertical())
					{
						current_road_ = road.get();

						MoveVertical(distance, ++iteration);
						return;
					}
				}
			}

			velocity_ = { 0,0 };
			if (desired_point < y1)
			{
				position_.y = y1;
			}
			else
			{
				position_.y = y2;
			}
			return;
		}

		position_.y = desired_point;
		return;
	}

	void Dog::MoveHorizontal(double distance, int iteration)
	{
		if (distance == 0 || iteration > 2)
			return;

		Point start = current_road_->GetStart();
		Point end = current_road_->GetEnd();

		double desired_point = position_.x + distance;

		double small_x, big_x;

		if (start.x < end.x)
		{
			small_x = start.x;
			big_x = end.x;
		}
		else
		{
			small_x = end.x;
			big_x = start.x;
		}

		double x1 = small_x - 0.4;
		double x2 = big_x + 0.4;

		if (desired_point < x1 || desired_point > x2)
		{
			for (std::shared_ptr<Road> road : GetRoadsByPos(position_))
			{
				if (road != nullptr)
				{
					if (road.get() != current_road_ && road.get()->IsHorizontal())
					{
						current_road_ = road.get();

						MoveHorizontal(distance, ++iteration);
						return;
					}
				}
			}

			velocity_ = { 0,0 };
			if (desired_point < x1)
			{
				position_.x = x1;
			}
			else
			{
				position_.x = x2;
			}
			return;
		}

		position_.x = desired_point;
		return;
	}

	void Dog::Move(double ms)
	{
		double distance = ms / 1000;
		bool is_vertical = velocity_.x == 0 && velocity_.y != 0;
		bool is_horizontal = velocity_.x != 0 && velocity_.y == 0;

		if (is_vertical)
		{
			MoveVertical(distance * velocity_.y, 0);
		}

		if (is_horizontal)
		{
			MoveHorizontal(distance * velocity_.x, 0);
		}
	}

	//Players

	std::string Players::MakePlayer(std::string username, const Map* map)
	{
		Player player{ BirthDog(map), players_.size(), username, map, *this};
		players_.push_back(std::move(player));

		Player* ptr = &players_.back();

		std::string token{ std::move(GenerateToken()) };

		//Protection against impossible odds of two identical tokens generating. What a waste :p
		while (FindPlayerByToken(token) != nullptr)
		{
			//You should buy a lottery ticket!
			token = std::move(GenerateToken());
		}

		token_to_player_.emplace(token, ptr);
		map_id_to_players_[*map->GetId()].push_back(ptr);

		return token;
	}

	Player* Players::FindPlayerByIdx(size_t idx)
	{
		if (idx < players_.size())
		{
			return &players_[idx];
		}

		return nullptr;
	}
	Player* Players::FindPlayerByToken(std::string token) const
	{
		std::transform(token.begin(), token.end(), token.begin(), [](unsigned char c)
			{
				return std::tolower(c);
			});

		if (token_to_player_.contains(token))
		{
			return token_to_player_.at(token);
		}

		return nullptr;
	}

	Dog* Players::BirthDog(const Map* map)
	{
		//Finding the suitable road for the puppy to be born
		const std::vector<Road>& roads = map->GetRoads();

		//Now we find the exact spot on that road
		Coordinates spot = map->GetRandomSpot();

		//Temporary setting dog spot to the start of the first road
		if (!randomize_)
		{
			Point p{ roads.at(0).GetStart() };
			spot.x = p.x;
			spot.y = p.y;
		}

		Dog pup{ spot, map };
		dogs_.push_back(std::move(pup));
		return &dogs_.back();
	}

	Dog* Players::FindDogByIdx(size_t idx)
	{
		if (idx >= 0 || idx < dogs_.size())
		{
			return &dogs_[idx];
		}

		return nullptr;
	}

	const int Players::GetPlayerCount(std::string map_id) const
	{
		if (map_id_to_players_.contains(map_id))
		{
			return map_id_to_players_.at(map_id).size();
		}
		else
		{
			return 0;
		}
	}

	void Players::MoveAllByMap(double ms, std::string& id)
	{
		for (Player* player : map_id_to_players_[id])
		{
			if (player != nullptr)
			{
				player->Move(ms);
			}
		}
	}

	std::deque<Coordinates> Players::GetLooterPositionsByMap(const std::string& id) const
	{
		std::deque<Coordinates> result;

		if (map_id_to_players_.contains(id))
		{
			for (Player* player : map_id_to_players_.at(id))
			{
				if(player != nullptr)
				{
					if (player->GetItemCount() < player->GetCurrentMap()->GetBagCapacity())
					{
						result.push_back(player->GetPos());
					}
				}
			}
		}

		return result;
	}

	Dog* Players::InsertDog(const Dog& dog)
	{
		dogs_.push_back(dog);
		return &dogs_.back();
	}

	void Players::InsertPlayer(Player& player, const std::string& token, const std::string& map_id)
	{
		players_.push_back(std::move(player));

		Player* player_ptr = &players_.back();

		token_to_player_.emplace(token, player_ptr);
		map_id_to_players_[map_id].push_back(player_ptr);
	}		
	
	void Players::RemovePlayer(Player* pl)
	{
		for (auto& entry : token_to_player_)
		{
			if (pl == entry.second)
			{
				token_to_player_.erase(entry.first);
				break;
			}
		}
	}

	//===Player===
	
	void Player::StoreItem(const Item& item)
	{
		bag_.push_back(item);

		json::object logger_data;
		logger_data.emplace("Player ID", id_);
		logger_data.emplace("Item ID", item.id);
		logger_data.emplace("Type", item.type);
		logger_data.emplace("Value", item.value);

		logger_data.emplace("Total Count", bag_.size());


		BOOST_LOG_TRIVIAL(info) << logging::add_value(timestamp, pt::microsec_clock::local_time()) << logging::add_value(additional_data, logger_data) << "collected item";
	}

	void Player::Depot()
	{
		score_ = std::accumulate(bag_.begin(), bag_.end(), score_, [](int total, const Item& item) { return total + item.value; });

		bag_.clear();
	}

	void Player::Retire(int64_t current_age)
	{
		if (!is_removed)
		{
			is_removed = true;
			current_map_->RetireDog(username_, score_, current_age);
			player_manager_.RemovePlayer(this);
		}
	}
	void Player::SetVel(double vel_x, double vel_y)
	{
		if (vel_x != 0 && vel_y != 0)
		{
			idle_time = 0;
		}

		pet_->SetVel(vel_x, vel_y);
	}

	void Player::Move(int ms)
	{
		age_ms_ += ms;

		Velocity vel = pet_->GetVel();

		if (vel.x == 0 && vel.y == 0)
		{
			idle_time += ms;

			if (idle_time >= current_map_->GetAFK())
			{
				age_ms_ -= idle_time - current_map_->GetAFK();
				Retire(age_ms_);
			}
		}
		else
		{
			idle_time = 0;
			pet_->Move(ms);
		}
	}

	//===Item===
	Item::Item(Coordinates pos_, int id_, int type_, int64_t value_)
		:pos(pos_),
		id(id_),
		type(type_),
		value(value_)
	{}

	Item::Item(Coordinates pos_, double width_, int id_, int type_, int64_t value_)
		:pos(pos_),
		width(width_),
		id(id_),
		type(type_),
		value(value_)
	{}

}  // namespace model
