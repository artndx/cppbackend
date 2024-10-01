#pragma once;

#include <boost/serialization/vector.hpp>
#include <boost/serialization/deque.hpp>

#include "model.h"

namespace model
{
	template <typename Archive>
	void serialize(Archive& ar, Point& point, [[maybe_unused]] const unsigned version)
	{
		ar& point.x;
		ar& point.y;
	}

	template <typename Archive>
	void serialize(Archive& ar, Coordinates& pos, [[maybe_unused]] const unsigned version)
	{
		ar& pos.x;
		ar& pos.y;
	}

	template <typename Archive>
	void serialize(Archive& ar, Velocity& vel, [[maybe_unused]] const unsigned version)
	{
		ar& vel.x;
		ar& vel.y;
	}

	template <typename Archive>
	void serialize(Archive& ar, Item& item, [[maybe_unused]] const unsigned version)
	{
		ar& item.pos;
		ar& item.width;
		ar& item.id;
		ar& item.type;
		ar& item.value;
	}
}  // namespace model

namespace serialization
{

	// DogRepr (DogRepresentation) - сериализованное представление класса Dog
	class DogRepr
	{
	public:
		DogRepr() = default;

		explicit DogRepr(const model::Dog& dog)
			: position_(dog.GetPos()),
			current_map_(dog.GetCurrentMap()),
			current_map_id_(current_map_->GetId()),
			current_road_(dog.GetCurrentRoad()),
			road_start_(current_road_->GetStart()),
			road_end_(current_road_->GetEnd()),
			velocity_(dog.GetVel()),
			direction_(dog.GetDir()),
			speed_(current_map_->GetDogSpeed()){}

		[[nodiscard]] model::Dog Restore(model::Game& game) const
		{
			const model::Map* the_map = game.FindMap(current_map_id_);

			model::Road* the_road;
			// :(
			for (auto& road : the_map->GetRoadsAtPoint(road_start_))
			{
				model::Road* tmp = road.get();

				if (tmp->GetStart() == road_start_ && tmp->GetEnd() == road_end_)
				{
					the_road = tmp;
					break;
				}
			}

			model::Dog dog{ position_, the_road, velocity_, speed_, direction_, the_map };
			return dog;
		}

		template <typename Archive>
		void serialize(Archive& ar, [[maybe_unused]] const unsigned version)
		{
			ar& *current_map_id_;
			ar& position_;
			ar& road_start_;
			ar& road_end_;
			ar& velocity_;
			ar& speed_;
			ar& direction_;
		}

	private:
		model::Coordinates position_ = {-1, -1};

		const model::Map* current_map_ = nullptr;
		model::Map::Id current_map_id_ = model::Map::Id("id");

		model::Road* current_road_ = nullptr;
		model::Point road_start_ = { -1, -1 };
		model::Point road_end_ = { -1, -1 };

		model::Velocity velocity_ = { -1, -1 };;
		model::Direction direction_ = model::Direction::NORTH;
		double speed_ = -1;
	};

	class PlayerRepr
	{
	public:
		PlayerRepr() = default;

		explicit PlayerRepr(const model::Player& player)
			:pet_(player.GetDog()),
			id_(player.GetId()),
			username_(player.GetName()),
			current_map_(pet_->GetCurrentMap()),
			current_map_id_(current_map_->GetId()),
			bag_(player.PeekInTheBag()),
			score(player.GetScore()){}

		[[nodiscard]] model::Player Restore(model::Game& game, model::Dog* lost_pup, model::Players& pm) const
		{
			const model::Map* the_map = game.FindMap(current_map_id_);

			return { lost_pup, id_, username_, the_map, score, bag_, pm };
		}

		template <typename Archive>
		void serialize(Archive& ar, [[maybe_unused]] const unsigned version)
		{
			ar& *current_map_id_;

			ar& id_;
			ar& username_;

			ar& score;
			ar& bag_;
		}

	private:

		model::Dog* pet_ = nullptr;		

		size_t id_ = -1;
		std::string username_ = "";

		const model::Map* current_map_ = nullptr;
		model::Map::Id current_map_id_{model::Map::Id("id")};

		std::deque<model::Item> bag_ = {};

		int64_t score = -1;

	};

}  // namespace serialization
