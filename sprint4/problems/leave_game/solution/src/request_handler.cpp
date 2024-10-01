#include "request_handler.h"

bool IsValidToken(std::string token)
{
	if (token.size() != 32)
	{
		return false;
	}

	return true;
}

namespace http_handler 
{
	StringResponse MakeStringResponse(http::status status, std::string_view body, 
		unsigned http_version, bool keep_alive, std::string_view content_type)
	{
		StringResponse response(status, http_version);
		response.set(http::field::content_type, content_type);
		response.set(http::field::cache_control, "no-cache");
		response.body() = body;
		response.content_length(body.size());
		response.keep_alive(keep_alive);
		return response;
	}

	FileResponse MakeResponse(http::status status, http::file_body::value_type& body, 
		unsigned http_version, bool keep_alive, std::string_view content_type)
	{
		FileResponse response;
		response.version(http_version);
		response.result(status);
		response.set(http::field::content_type, content_type);
		response.body() = std::move(body);
		response.prepare_payload();
		response.keep_alive(keep_alive);
		return response;
	}

	void PackMaps(json::array& target_container, model::Game& game)
	{
		for (auto& map : game.GetMaps())
		{
			json::object elem;

			elem.emplace("id", *map.GetId());
			elem.emplace("name", map.GetName());

			target_container.push_back(std::move(elem));
		}
	}


	void PackRoads(json::array& target_container, const model::Map* map_ptr)
	{
		for (const model::Road& road_data : map_ptr->GetRoads())
		{
			//Temporary road object
			json::object road;

			model::Point start = road_data.GetStart();
			model::Point end = road_data.GetEnd();

			road.emplace(X0_COORDINATE, start.x);
			road.emplace(Y0_COORDINATE, start.y);

			if (road_data.IsHorizontal())
			{
				road.emplace(X1_COORDINATE, end.x);
			}
			else
			{
				road.emplace(Y1_COORDINATE, end.y);
			}

			//Pushing road to the road container
			target_container.push_back(std::move(road));
		}
	}

	void PackBuildings(json::array& target_container, const model::Map* map_ptr)
	{
		for (const model::Building& building_data : map_ptr->GetBuildings())
		{
			//Temporary building object
			json::object building;

			model::Rectangle dimensions = building_data.GetBounds();

			building.emplace(X_COORDINATE, dimensions.position.x);
			building.emplace(Y_COORDINATE, dimensions.position.y);
			building.emplace(WIDTH, dimensions.size.width);
			building.emplace(HEIGHT, dimensions.size.height);

			//Pushing building to the building container
			target_container.push_back(std::move(building));
		}
	}

	void PackOffices(json::array& target_container, const model::Map* map_ptr)
	{
		for (const model::Office& office_data : map_ptr->GetOffices())
		{
			//Temporary office object
			json::object office;

			office.emplace("id", *office_data.GetId());

			model::Point pos = office_data.GetPosition();
			office.emplace(X_COORDINATE, pos.x);
			office.emplace(Y_COORDINATE, pos.y);

			model::Offset offset = office_data.GetOffset();
			office.emplace(X_OFFSET, offset.dx);
			office.emplace(Y_OFFSET, offset.dy);

			//Pushing office to the building container
			target_container.push_back(std::move(office));
		}
	}

	bool IsSubPath(fs::path path, fs::path base)
	{
		path = fs::weakly_canonical(path);
		base = fs::weakly_canonical(base);

		for (auto b = base.begin(), p = path.begin(); b != base.end(); ++b, ++p)
		{
			if (p == path.end() || *p != *b)
			{
				return false;
			}
		}

		return true;
	}

	//Dictionary with different extensions. Idk where else to put it
	std::map<std::string_view, std::string_view> EXTENSION_TO_VALUE =
	{
		{".json"sv, ContentType::APPLICATION_JSON},
		{".xml"sv, ContentType::APPLICATION_XML},

		{".js"sv, ContentType::TEXT_JS},
		{".css"sv, ContentType::TEXT_CSS},
		{".txt"sv, ContentType::TEXT_TXT},
		{".htm"sv, ContentType::TEXT_HTML},
		{".html"sv, ContentType::TEXT_HTML},

		{".png"sv, ContentType::IMAGE_PNG},
		{".jpg"sv, ContentType::IMAGE_JPG},
		{".jpe"sv, ContentType::IMAGE_JPG},
		{".jpeg"sv, ContentType::IMAGE_JPG},
		{".gif"sv, ContentType::IMAGE_GIF},
		{".bmp"sv, ContentType::IMAGE_BMP},
		{".ico"sv, ContentType::IMAGE_ICO},
		{".tif"sv, ContentType::IMAGE_TIF},
		{".tiff"sv, ContentType::IMAGE_TIF},
		{".svg"sv, ContentType::IMAGE_SVG},
		{".svgz"sv, ContentType::IMAGE_SVG},

		{".mp3"sv, ContentType::AUDIO_MP3},
	};

	std::string_view GetContentType(std::string_view extension)
	{
		auto iter = EXTENSION_TO_VALUE.find(extension);

		if (iter != EXTENSION_TO_VALUE.end())
		{
			return iter->second;
		}

		return ContentType::APPLICATION_BLANK;
	}

}  // namespace http_handler
