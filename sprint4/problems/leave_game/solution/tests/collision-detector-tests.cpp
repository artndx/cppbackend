#include <catch2/catch_test_macros.hpp>
#define _USE_MATH_DEFINES

#include "../src/collision_detector.h"
#include <string>
#include <iostream>
#include <cmath>
#include <sstream>

using namespace std::literals;

namespace Catch 
{
    template<>
    struct StringMaker<collision_detector::GatheringEvent> 
    {
        static std::string convert(collision_detector::GatheringEvent const& value) 
        {
            std::ostringstream tmp;
            tmp << "(" << value.gatherer_id << "," << value.item_id << "," << value.sq_distance << "," << value.time << ")";

            return tmp.str();
        }
    };

    bool operator==(const collision_detector::CollectionResult& result, std::pair<double, double> values)
    {
        return result.sq_distance == values.second && result.proj_ratio == values.first;
    }

    class VectorItemGathererProvider : public collision_detector::ItemGathererProvider 
    {
        public:
        
        VectorItemGathererProvider(std::vector<collision_detector::Item> items, std::vector<collision_detector::Gatherer> gatherers)
            :items_(items),
            gatherers_(gatherers) 
        {}


        size_t ItemsCount() const override 
        {
            return items_.size();
        }

        collision_detector::Item GetItem(size_t idx) const override 
        {
            return items_[idx];
        }

        size_t GatherersCount() const override 
        {
            return gatherers_.size();
        }

        collision_detector::Gatherer GetGatherer(size_t idx) const override 
        {
            return gatherers_[idx];
        }

        private:
        
            std::vector<collision_detector::Item> items_;
            std::vector<collision_detector::Gatherer> gatherers_;
    };

    TEST_CASE("Collect Point Test", "[collision detector]")
    {
        REQUIRE(collision_detector::TryCollectPoint({ 0, 0 }, { 1, 1 }, { 0, 1 }) == std::pair{ 0.5, 0.5 });
    }

    TEST_CASE("Find Gather Events Test #1", "[no interaction]")
    {
        std::vector<collision_detector::Gatherer> gatherers{ {{0, 0}, {0, 5}, 3.}, {{5, 0}, {0, 0}, 3.}, {{0, 7}, {7, 7}, 5.} };
        std::vector<collision_detector::Item> items{ {{10, 0}, 3.}, {{5, 5}, 3.}, {{4, 7}, 5.} };

        VectorItemGathererProvider first_provider{ {}, gatherers };

        //No items on the map
        REQUIRE(collision_detector::FindGatherEvents(first_provider).size() == 0);


        VectorItemGathererProvider second_provider{ items, {} };

        //No items on the map
        REQUIRE(collision_detector::FindGatherEvents(second_provider).size() == 0);


        VectorItemGathererProvider third_provider{ items, {{{0,0}, {0, 0}, 3.}, {{10, 10}, {10, 10}, 3.} } };

        //Gatherers stay in place
        REQUIRE(collision_detector::FindGatherEvents(third_provider).size() == 0);
    };

    TEST_CASE("Find Gather Events Test #2", "[pickups]")
    {
        std::vector<collision_detector::Item> items{ {{0, 5}, 3.}, {{0, 10}, 3.}, {{0, 15}, 3.} };

        VectorItemGathererProvider first_provider{ items, {{{0,0}, {0, 20}, 3.}} };

        //Multiple Items Picked Up
        auto events_first = collision_detector::FindGatherEvents(first_provider);
        REQUIRE(events_first.size() != 0);

        for (int i = 0; i < events_first.size(); ++i)
        {
            CHECK(events_first[i].gatherer_id == 0);
            CHECK(events_first[i].item_id == i);
        }

        VectorItemGathererProvider second_provider{ items, {{{0,0}, {0, 20}, 3.}, {{0, 1}, {0,10}, 3.}} };

        //Multiple Items Picked Up But One Gatherer Is Faster
        auto events_second = collision_detector::FindGatherEvents(second_provider);
        REQUIRE(events_second.size() != 0);

        CHECK(events_second[2].gatherer_id == 0);
        CHECK(events_second[2].item_id == 1);

        VectorItemGathererProvider third_provider{ {{{5, 5}, 3. }}, {{ {0,0}, {10, 10}, 3. }} };

        //Diagonal Pickup
        REQUIRE(collision_detector::FindGatherEvents(second_provider).size() != 0);
    };

}  // namespace Catch