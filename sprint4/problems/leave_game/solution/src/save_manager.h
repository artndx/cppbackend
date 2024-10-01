#pragma once

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <sstream>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <cstdio>

#include "model.h"
#include "model_serialization.h"

using namespace model;
using namespace std::literals;

namespace
{

	using InputArchive = boost::archive::text_iarchive;
	using OutputArchive = boost::archive::text_oarchive;

	struct Fixture
	{
		std::stringstream strm;
		OutputArchive output_archive{ strm };
	};

}

namespace savesystem
{
	class SaveManager
	{
	public:

		SaveManager(std::string filepath, int save_period, model::Game& game)
			:filepath_(std::filesystem::path(filepath)),
			save_period_(save_period),
			game_(game) {}

		void Listen(int ms)
		{
			if (save_period_ < 0)
				return;

			ms_since_last_call += ms;

			if (ms_since_last_call >= save_period_)
			{
				SaveState();
			}
		}

		void LoadState()
		{
			if (!std::filesystem::exists(filepath_))
			{
				//std::ofstream blank_savefile(filepath_);
				//blank_savefile.close();
				return;
			}

			std::ifstream stream{ filepath_ };
			InputArchive input_archive{ stream };

			//First we load in all the lootable items
			size_t iterations;
			input_archive >> iterations;

			for (size_t i = 0; i < iterations; ++i)
			{
				std::string map_id;
				std::deque<model::Item> items;
				input_archive >> map_id >> items;

				game_.SetLootOnMap(items, map_id);

				size_t player_count;
				input_archive >> player_count;

				for (int e = 0; e < player_count; ++e)
				{
					serialization::DogRepr dog_repr;
					serialization::PlayerRepr player_repr;

					input_archive >> dog_repr;

					model::Players& player_manager = game_.GetPlayerManager();

					auto restored_dog = dog_repr.Restore(game_);
					model::Dog* pup = player_manager.InsertDog(restored_dog);

					input_archive >> player_repr;

					std::string token;
					input_archive >> token;

					model::Player restored_player = player_repr.Restore(game_, pup, player_manager);

					player_manager.InsertPlayer(restored_player, token, map_id);
				}
			}
		}

		void SaveState()
		{
			//std::filesystem::path temp_path = filepath_;

			//std::string temp_name = " ";

			//temp_name += filepath_.filename().string();
			//temp_path.replace_filename(temp_name);

			//std::ofstream stream{ temp_path };
			std::ofstream stream{ filepath_ };
			OutputArchive output_archive{ stream };

			const std::vector<model::Map>& maps = game_.GetMaps();
			const Players& player_manager = game_.GetPlayerManager();

			//Steps to save a game state:

			//1. Save the total amount of maps
			output_archive << maps.size();
			
			//2. For each map we save it's ID, list of items and player data
			for (const model::Map& map : maps)
			{
				model::Map::Id map_id = map.GetId();

				//3. Storing the map ID and item list
				output_archive << *map_id << map.GetItemList();

				//4. Getting the list of active player
				const std::deque<Player*> players = game_.GetPlayerList(*map_id);

				//5. Storing the player count
				output_archive << players.size();

				//For each player we have to save the dog first, so we can point to it later
				for (const Player* player : game_.GetPlayerList(*map_id))
				{
					const Dog* dog = player->GetDog();

					serialization::DogRepr dog_repr{ *dog };
					serialization::PlayerRepr player_repr{ *player };

					//6. Saving the dog and player
					output_archive << dog_repr << player_repr;

					//7. Storing the token
					std::string token = "InvalidToken";

					for (const auto& entry : player_manager.GetTokenToPlayerTable())
					{
						if (entry.second == player)
						{
							token = entry.first;
							break;
						}
					}

					output_archive << token;
				}
			}
			stream.close();

			//8. Rename the savefile
			//remove(filepath_);
			//std::filesystem::rename(temp_path, filepath_);
		}

	private:

		model::Game& game_;

		std::filesystem::path filepath_;
		int save_period_;

		int ms_since_last_call = 0;
	};
}
