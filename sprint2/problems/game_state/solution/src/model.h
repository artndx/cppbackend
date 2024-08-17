#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <deque>

#include "tagged.h"

namespace model {

using Dimension = int;
using Coord = Dimension;

struct Point {
    Coord x, y;
};

struct Size {
    Dimension width, height;
};

enum class Direction{
    NORTH,
    SOUTH,
    WEST,
    EAST
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
        : start_{start}
        , end_{end_x, start.y} {
    }

    Road(VerticalTag, Point start, Coord end_y) noexcept
        : start_{start}
        , end_{start.x, end_y} {
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

private:
    Point start_;
    Point end_;
};

class Building {
public:
    explicit Building(Rectangle bounds) noexcept
        : bounds_{bounds} {
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
        : id_{std::move(id)}
        , position_{position}
        , offset_{offset} {
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

class Dog{
public:
    using Name = util::Tagged<std::string, Dog>;
    using Position = util::Tagged<std::pair<double, double>, Dog>;
    using Speed = util::Tagged<std::pair<double, double>, Dog>;

    Dog(int id, Name name, Position pos, Speed speed, Direction dir) noexcept
        : id_(id), name_(name)
        , pos_(pos), speed_(speed), dir_(dir){
    }

    int GetId() const{
        return id_;
    }

    const Name& GetName() const{
        return name_;
    }

    const Position& GetPosition() const{
        return pos_;
    }

    const Speed& GetSpeed() const{
        return speed_;
    }

    Direction GetDirection() const{
        return dir_;
    }
private:
    int id_;
    Name name_;
    Position pos_;
    Speed speed_;
    Direction dir_;
};

class Map {
public:
    using Id = util::Tagged<std::string, Map>;
    using Roads = std::vector<Road>;
    using Buildings = std::vector<Building>;
    using Offices = std::vector<Office>;

    Map(Id id, std::string name) noexcept
        : id_(std::move(id))
        , name_(std::move(name)) {
    }

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

    void AddBuilding(const Building& building) {
        buildings_.emplace_back(building);
    }

    void AddOffice(Office office);

private:
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

    Id id_;
    std::string name_;
    Roads roads_;
    Buildings buildings_;

    OfficeIdToIndex warehouse_id_to_index_;
    Offices offices_;
};

class GameSession{
public:
    GameSession(const Map* map)
        : map_(map){
    }

    const Dog* AddDog(int id, const Dog::Name& name, 
                        const Dog::Position& pos, const Dog::Speed& vel, 
                        Direction dir){
        dogs_.emplace_back(id, name, pos, vel, dir);
        return &dogs_.back();
    }

    const Map* GetMap() const {
        return map_;
    }
private:
    std::deque<Dog> dogs_;
    const Map* map_;
};

class Game {
public:
    using Maps = std::vector<Map>;

    void AddMap(Map map);

    GameSession* AddSession(const Map::Id& map);

    GameSession* SessionIsExists(const Map::Id& id);

    const Maps& GetMaps() const noexcept {
        return maps_;
    }

    const Map* FindMap(const Map::Id& id) const noexcept {
        if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
            return &maps_.at(it->second);
        }
        return nullptr;
    }

private:
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;
    using SessionsByMapId = std::unordered_map<Map::Id, std::deque<GameSession>, MapIdHasher>;

    Maps maps_;
    SessionsByMapId sessions_;
    MapIdToIndex map_id_to_index_;
};

}  // namespace model
