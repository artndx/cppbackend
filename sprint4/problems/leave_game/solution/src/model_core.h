#pragma once

#include <string>
#include <set>
#include <unordered_map>
#include <vector>
#include <deque>
#include <iostream>
#include <chrono>
#include <memory>
#include <algorithm>
#include <optional>
#include <boost/signals2.hpp>
#include <stdexcept>

#include "collision_detector.h"
#include "loot_generator.h"
#include "extra_data.h"
#include "tagged_uuid.h"
#include "DB_manager.h"

#include "http_server.h"

using pqxx::operator"" _zv;

const std::string X_COORDINATE = "x";
const std::string Y_COORDINATE = "y";


const std::string X_OFFSET = "offsetX";
const std::string Y_OFFSET = "offsetY";

const std::string X0_COORDINATE = "x0";
const std::string Y0_COORDINATE = "y0";
const std::string X1_COORDINATE = "x1";
const std::string Y1_COORDINATE = "y1";

const std::string WIDTH = "w";
const std::string HEIGHT = "h";

int GetRandomNumber(int range);

std::string GenerateToken();

namespace model
{

	using Dimension = int;
	using Coord = Dimension;

	struct Point
	{
		Coord x, y;

		bool operator==(const Point& other) const
		{
			return (x == other.x && y == other.y);
		}
	};

	struct PointHasher
	{
		std::size_t operator()(const Point& p) const
		{
			using std::size_t;

			return p.x + p.y * p.y;
		}
	};

	struct Size {
		Dimension width, height;
	};

	struct Rectangle {
		Point position;
		Size size;
	};

	struct Offset {
		Dimension dx, dy;
	};


	class Road {
		struct HorizontalTag {
			explicit HorizontalTag() = default;
		};

		struct VerticalTag {
			explicit VerticalTag() = default;
		};

	public:
		constexpr static HorizontalTag HORIZONTAL{};
		constexpr static VerticalTag VERTICAL{};

		Road(HorizontalTag, Point start, Coord end_x) noexcept
			: start_{ start }
			, end_{ end_x, start.y } {
		}

		Road(VerticalTag, Point start, Coord end_y) noexcept
			: start_{ start }
			, end_{ start.x, end_y } {
		}

		bool IsHorizontal() const noexcept {
			return start_.y == end_.y;
		}

		bool IsVertical() const noexcept {
			return start_.x == end_.x;
		}

		Point GetStart() const noexcept {
			return start_;
		}

		Point GetEnd() const noexcept {
			return end_;
		}

		friend bool operator==(const Road& r1, const Road& r2);

	private:
		Point start_;
		Point end_;
	};

	class Building {
	public:
		explicit Building(Rectangle bounds) noexcept
			: bounds_{ bounds } {
		}

		const Rectangle& GetBounds() const noexcept {
			return bounds_;
		}

	private:
		Rectangle bounds_;
	};

	class Office {
	public:
		using Id = util::Tagged<std::string, Office>;

		Office(Id id, Point position, Offset offset) noexcept
			: id_{ std::move(id) }
			, position_{ position }
			, offset_{ offset } {
		}

		const Id& GetId() const noexcept {
			return id_;
		}

		Point GetPosition() const noexcept {
			return position_;
		}

		Offset GetOffset() const noexcept {
			return offset_;
		}

	private:
		Id id_;
		Point position_;
		Offset offset_;
	};


	struct Coordinates
	{
		double x;
		double y;
	};

	struct Item
	{
		Item() = default;

		Item(Coordinates pos_, int id_, int type_, int64_t value_);

		Item(Coordinates pos_, double width_, int id_, int type_, int64_t value_);

		Coordinates pos;
		double width = 0;
		int id;
		int type;
		int64_t value;
	};

	enum Direction
	{
		NORTH = 'U',
		SOUTH = 'D',
		WEST = 'L',
		EAST = 'R'
	};

	struct Velocity
	{
		double x;
		double y;
	};

	class Map
	{
	public:
		using Id = util::Tagged<std::string, Map>;
		using Roads = std::vector<Road>;
		using Buildings = std::vector<Building>;
		using Offices = std::vector<Office>;

		Map(Id id, std::string name, db::ConnectionPool& pool) noexcept
			: id_(std::move(id))
			, name_(std::move(name))
			, connection_pool_(pool)
		{}

		const Id& GetId() const noexcept {
			return id_;
		}

		const std::string& GetName() const noexcept {
			return name_;
		}

		const Buildings& GetBuildings() const noexcept {
			return buildings_;
		}

		const Roads& GetRoads() const noexcept {
			return roads_;
		}

		const Offices& GetOffices() const noexcept {
			return offices_;
		}

		void AddRoad(const Road& road) {
			roads_.emplace_back(road);
		}

		const Road& FindRoad(Point start, Point end) const;

		void AddBuilding(const Building& building) {
			buildings_.emplace_back(building);
		}

		void SetDogSpeed(double speed)
		{
			dog_speed_ = speed;
		}

		void SetBagCapacity(int capacity)
		{
			bag_capacity_ = capacity;
		}

		void SetAFK(double threshold)
		{
			afk_threshold_ = threshold;
		}

		double GetAFK() const
		{
			return afk_threshold_;
		}

		Coordinates GetRandomSpot() const;

		double GetDogSpeed() const
		{
			return dog_speed_;
		}

		void CalcRoads();

		const std::deque<std::shared_ptr<Road>>& GetRoadsAtPoint(Point p) const
		{
			return point_to_roads_.at(p);
		}

		void GenerateItems(unsigned int amount, const Data::MapExtras& extras);

		void SetItems(const std::deque<Item>& items)
		{
			items_ = items;
		}

		int GetItemCount() const
		{
			return items_.size();
		}

		const std::deque<Item>& GetItemList() const
		{
			return items_;
		}

		Item GetItemByIdx(int idx) const
		{
			return items_[idx];
		}

		void RemoveItem(int id)
		{
			items_.erase(items_.begin() + id);
		}

		int GetBagCapacity() const
		{
			return bag_capacity_;
		}

		void AddOffice(Office office);

		void RetireDog(const std::string& username, int64_t score, int64_t time_alive) const;

	private:
		using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

		Id id_;
		std::string name_;
		Roads roads_;
		Buildings buildings_;

		std::unordered_map<Point, std::deque<std::shared_ptr<Road>>, PointHasher> point_to_roads_;

		std::deque<Item> items_;

		double dog_speed_ = 1;
		int bag_capacity_ = 3;

		OfficeIdToIndex warehouse_id_to_index_;
		Offices offices_;
		std::deque<Coordinates> buffer_;
		double afk_threshold_ = 60000.0;

		db::ConnectionPool& connection_pool_;
	};

} //~namespace model