#pragma once

#include "model_core.h"

namespace model
{
	class Dog
	{
	public:

		Dog(Coordinates coords, const Map* map);
		Dog(Coordinates coords, Road* current_road, Velocity vel, double speed, Direction dir, const Map* map);

		void SetVel(double vel_x, double vel_y)
		{
			velocity_.x = speed_ * vel_x;
			velocity_.y = speed_ * vel_y;
		}

		void SetDir(Direction dir)
		{
			direction_ = dir;
		}

		Coordinates GetPos() const
		{
			return position_;
		}

		Velocity GetVel() const
		{
			return velocity_;
		}

		Direction GetDir() const
		{
			return direction_;
		}

		const Map* GetCurrentMap() const
		{
			return current_map_;
		}

		Road* GetCurrentRoad() const
		{
			return current_road_;
		}

		const std::deque<std::shared_ptr<Road>>& GetRoadsByPos(Coordinates pos) const
		{
			int pos_x = static_cast<int>(pos.x + 0.5);
			int pos_y = static_cast<int>(pos.y + 0.5);

			return current_map_->GetRoadsAtPoint({ pos_x, pos_y });
		}

		void MoveVertical(double distance, int iteration); 
		
		void MoveHorizontal(double distance, int iteration);

		void Move(double ms);

	private:

		Coordinates position_;
		Road* current_road_;

		Velocity velocity_{ 0, 0 };
		double speed_;

		Direction direction_ = Direction::NORTH;

		const Map* current_map_;
	};

	class Players;

	class Player
	{
	public:
		Player(Dog* dog, size_t id, std::string username, const Map* maptr, Players& pm)
			:username_(username),
			current_map_(maptr),
			id_(id),
			pet_(dog),
			player_manager_(pm){}

		Player(Dog* dog, size_t id, std::string username, const Map* maptr, int64_t score, std::deque<Item> items, Players& pm)
			:username_(username),
			current_map_(maptr),
			bag_(items),
			id_(id),
			score_(score),
			pet_(dog),
			player_manager_(pm){}

		std::string GetName() const
		{
			return username_;
		}

		const Map* GetCurrentMap() const
		{
			return current_map_;
		}

		size_t GetId() const
		{
			return id_;
		}

		Coordinates GetPos() const
		{
			if (pet_ == nullptr)
				return { -1, -1 };

			return pet_->GetPos();
		}

		Velocity GetVel() const
		{
			if (pet_ == nullptr)
				return { -1, -1 };

			return pet_->GetVel();
		}

		Direction GetDir() const
		{
			if (pet_ == nullptr)
				return Direction::NORTH;

			return pet_->GetDir();
		}

		void SetVel(double vel_x, double vel_y);

		void SetDir(Direction dir)
		{
			pet_->SetDir(dir);
		}

		void Move(int ms);

		void Retire(int64_t current_age);

		void StoreItem(const Item& item);

		void Depot();

		const std::deque<Item> PeekInTheBag() const
		{
			return bag_;
		}

		int GetItemCount() const
		{
			return bag_.size();
		}		

		int64_t GetScore() const
		{
			return score_;
		}

		Dog* GetDog() const
		{
			return pet_;
		}

	private:

		std::string username_;
		const Map* current_map_;

		std::deque<Item> bag_;
		const size_t id_;

		int64_t score_ = 0;

		Dog* pet_;

		int idle_time = 0;
		int64_t age_ms_ = 0;

		Players& player_manager_;
		bool is_removed = false;
	};

	class Players
	{
	public:
		explicit Players(bool randomize)
			:randomize_(randomize) {}

		//This function creates a player and returns a token
		std::string MakePlayer(std::string username, const Map* map);

		Player* FindPlayerByIdx(size_t idx);
		Player* FindPlayerByToken(std::string token) const;

		Dog* BirthDog(const Map* map);
		Dog* FindDogByIdx(size_t idx);

		const std::deque<Player*> GetPlayerList(std::string map_id) const
		{
			return map_id_to_players_.at(map_id);
		}

		const int GetPlayerCount(std::string map_id) const;

		void MoveAllByMap(double ms, std::string& id);

		std::deque<Coordinates> GetLooterPositionsByMap(const std::string& id) const;

		Dog* InsertDog(const Dog& dog);

		void InsertPlayer(Player& pl, const std::string& token, const std::string& map_id);

		const std::unordered_map<std::string, Player*>& GetTokenToPlayerTable() const
		{
			return token_to_player_;
		}

		void RemovePlayer(Player* pl);

	private:

		bool randomize_;

		std::unordered_map<std::string, Player*> token_to_player_;
		std::unordered_map<std::string, std::deque<Player*>> map_id_to_players_;
		std::deque<Player> players_;
		std::deque<Dog> dogs_;

	};

	class Game
	{
	public:

		Game(Players& pm, db::ConnectionPool& pool)
			:player_manager_(pm),
			connection_pool_(pool){}

		using Maps = std::vector<Map>;

		void AddMap(Map map, double dog_speed = -1, int bag_capacity = -1);

		const Maps& GetMaps() const noexcept {
			return maps_;
		}

		const Map* FindMap(const Map::Id& id) const noexcept;

		std::string SpawnPlayer(std::string username, const Map* map)
		{
			return player_manager_.MakePlayer(username, map);
		}

		Player* FindPlayerByToken(std::string token) const
		{
			return player_manager_.FindPlayerByToken(token);
		}

		const std::deque<Player*> GetPlayerList(std::string map_id) const
		{
			return player_manager_.GetPlayerList(map_id);
		}

		const int GetPlayerCount(std::string map_id) const
		{
			return player_manager_.GetPlayerCount(map_id);
		}

		void SetGlobalDogSpeed(double speed)
		{
			global_dog_speed_ = speed;
		}

		void SetGlobalCapacity(int capacity)
		{
			global_bag_capacity = capacity;
		}

		void SetAFK(double threshold)
		{
			afk_threshold = threshold * 1000;
		}

		void MoveAndCalcPickups(Map& map, int ms);

		void ServerTick(int milliseconds);

		void SetExtraData(Data::MapExtras extras)
		{
			extra_data_ = extras;
		}

		boost::json::object GetLootConfig() const
		{
			return extra_data_.GetConfig();
		}

		boost::json::array GetLootTable(const std::string& id) const
		{
			return extra_data_.GetTable(id);
		}

		Players& GetPlayerManager()
		{
			return player_manager_;
		}

		void AddTable(const std::string& id, boost::json::array& table)
		{
			extra_data_.AddTable(id, table);
		}

		db::ConnectionPool& GetPool()
		{
			return connection_pool_;
		}

		void SetLootOnMap(const std::deque<Item>& items, const std::string& map_id);

	private:

		double global_dog_speed_ = 1;
		int global_bag_capacity = 3;
		double afk_threshold = 60000.0;

		using MapIdHasher = util::TaggedHasher<Map::Id>;
		using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

		db::ConnectionPool& connection_pool_;

		std::vector<Map> maps_;
		MapIdToIndex map_id_to_index_;

		Players& player_manager_;
		Data::MapExtras extra_data_;

		int save_period_ = -1;
		std::string save_file_ = "";
	};

}  // namespace model
