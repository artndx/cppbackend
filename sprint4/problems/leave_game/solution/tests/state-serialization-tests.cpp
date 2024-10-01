#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <catch2/catch_test_macros.hpp>
#include <sstream>
#include <iostream>

#include "../src/model.h"
#include "../src/model_serialization.h"

using namespace model;
using namespace std::literals;
namespace {

using InputArchive = boost::archive::text_iarchive;
using OutputArchive = boost::archive::text_oarchive;

struct Fixture {
    std::stringstream strm;
    OutputArchive output_archive{strm};
};

}  // namespace

SCENARIO_METHOD(Fixture, "Point serialization") 
{
    GIVEN("A point") 
    {
        const model::Point p{10, 20};
        WHEN("point is serialized") 
        {
            output_archive << p;

            THEN("it is equal to point after serialization") 
            {
                InputArchive input_archive{strm};
                model::Point restored_point;
                input_archive >> restored_point;
                CHECK(p == restored_point);
            }
        }
    }
}

SCENARIO_METHOD(Fixture, "Serialization") 
{
    Players player_manager{ false };

    Game game{ player_manager };
    
    Road road( Road::HORIZONTAL, Point{0, 50}, 100 );
    Map new_map{ Map::Id("map1"), "Map 1"};

    new_map.AddRoad(road);
    new_map.CalcRoads();

    game.AddMap(new_map);

    const Map* maptr = game.FindMap(Map::Id("map1"));

    Road* road_ptr = maptr->GetRoadsAtPoint(Point{ 0, 50 })[0].get();

    GIVEN("items")
    {
        //Dog(Coordinates coords, Road* current_road, Velocity vel, double speed, Direction dir, const Map* map);
        const auto item1 = []
            {
                Item item(Coordinates{ 69.0, 50.0 }, 0.3, 0, 0, 50);
                return item;
            }();

        const auto item2 = []
            {
                Item item(Coordinates{ 75.0, 50.0 }, 0.3, 1, 1, 30);
                return item;
            }();

        std::deque<Item> items{ item1, item2 };

        WHEN("item is serialized")
        {
            std::cout << "Single Item Serialization Test..\n";
            {
                output_archive << item1;
            }

            THEN("it can be deserialized")
            {
                InputArchive input_archive{ strm };
                Item repr;
                input_archive >> repr;

                CHECK(item1.pos.x == repr.pos.x);
                CHECK(item1.pos.y == repr.pos.y);
                CHECK(item1.id == repr.id);
                CHECK(item1.width == repr.width);
                CHECK(item1.type == repr.type);
                CHECK(item1.value == repr.value);
            }
        }

        WHEN("item container is serialized")
        {
            std::cout << "Item Container Serialization Test..\n";
            {
                output_archive << items.size();
                output_archive << items;
            }

            THEN("it can be deserialized")
            {
                InputArchive input_archive{ strm };

                size_t iterations;
                input_archive >> iterations;

                CHECK(iterations == 2);

                std::deque<Item> saved_items;

                input_archive >> saved_items;

                REQUIRE(items.size() == saved_items.size());

                for (int i = 0; i < items.size(); ++i)
                {
                    CHECK(items[i].pos.x == saved_items[i].pos.x);
                    CHECK(items[i].pos.y == saved_items[i].pos.y);
                    CHECK(items[i].id == saved_items[i].id);
                    CHECK(items[i].width == saved_items[i].width);
                    CHECK(items[i].type == saved_items[i].type);
                    CHECK(items[i].value == saved_items[i].value);
                }
            }
        }
    }

    GIVEN("a dog") 
    {
        std::cout << "Dog Serialization Test..\n";
        //Dog(Coordinates coords, Road* current_road, Velocity vel, double speed, Direction dir, const Map* map);
        const auto dog = [maptr, road_ptr]
        {
            Dog dog( Coordinates{69.0, 50.0}, road_ptr, Velocity{42.2, 12.5}, 3., Direction::SOUTH, maptr );
            return dog;
        }();

        WHEN("dog is serialized") 
        {
            {
                serialization::DogRepr repr{dog};
                output_archive << repr;
            }

            THEN("it can be deserialized") 
            {
                InputArchive input_archive{strm};
                serialization::DogRepr repr;
                input_archive >> repr;
                const auto restored = repr.Restore(game);

                CHECK(dog.GetPos().x == restored.GetPos().x);
                CHECK(dog.GetPos().y == restored.GetPos().y);
                CHECK(dog.GetCurrentRoad()->GetStart() == restored.GetCurrentRoad()->GetStart());
                CHECK(dog.GetCurrentRoad()->GetEnd() == restored.GetCurrentRoad()->GetEnd());
                CHECK(dog.GetVel().x == restored.GetVel().x);
                CHECK(dog.GetVel().y == restored.GetVel().y);
                CHECK(dog.GetDir() == restored.GetDir());
                CHECK(dog.GetCurrentMap() == restored.GetCurrentMap());
            }
        }
    }

    GIVEN("a player")
    {
        std::cout << "Player Serialization Test..\n";

        //Player should always have a dog serialized before them
        player_manager.MakePlayer("Johny", maptr);

        Dog* dog = player_manager.FindDogByIdx(0);
        dog->SetDir(Direction::SOUTH);
        dog->SetVel(42.2, 12.5);

        Player* player = player_manager.FindPlayerByIdx(0);

        player->StoreItem({ {5.0, 15.0}, 0.3, 0, 1, 50 });
        player->StoreItem({ {5.0, 20.0}, 0.3, 1, 1, 50 });
        player->StoreItem({ {5.0, 25.0}, 0.3, 2, 0, 30 });

        WHEN("player is serialized")
        {
            {
                serialization::DogRepr dog_repr{ *dog };
                serialization::PlayerRepr player_repr{ *player };
                output_archive  << dog_repr << player_repr;
            }

            THEN("it can be deserialized")
            {
                InputArchive input_archive{ strm };
                serialization::DogRepr dog_repr;
                serialization::PlayerRepr player_repr;

                input_archive >> dog_repr;

                auto restored_dog = dog_repr.Restore(game);

                input_archive >> player_repr;

                const auto restored_player = player_repr.Restore(game, &restored_dog );

                CHECK(player->GetName() == restored_player.GetName());
                CHECK(*player->GetCurrentMap()->GetId() == *restored_player.GetCurrentMap()->GetId());
                CHECK(player->GetId() == restored_player.GetId());
                CHECK(player->GetPos().x == restored_player.GetPos().x);
                CHECK(player->GetPos().y == restored_player.GetPos().y);
                CHECK(player->GetVel().x == restored_player.GetVel().x);
                CHECK(player->GetVel().y == restored_player.GetVel().y);
                CHECK(player->GetDir() == restored_player.GetDir());
                CHECK(player->GetScore() == restored_player.GetScore());

                CHECK(player->GetItemCount() == restored_player.GetItemCount());

                for (int i = 0; i < player->GetItemCount(); ++i)
                {
                    const std::deque<Item>& bag = player->PeekInTheBag();
                    const std::deque<Item>& other_bag = restored_player.PeekInTheBag();

                    CHECK(bag[i].pos.x == other_bag[i].pos.x);
                    CHECK(bag[i].pos.y == other_bag[i].pos.y);
                    CHECK(bag[i].id == other_bag[i].id);
                    CHECK(bag[i].width == other_bag[i].width);
                    CHECK(bag[i].type == other_bag[i].type);
                    CHECK(bag[i].value == other_bag[i].value);
                }
            }
        }
    }

    std::cout << "Enter anything at all to close the tab..\n";
    std::string s;
    std::cin >> s;
}    
