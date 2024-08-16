#pragma once
#include <boost/regex.hpp>
#include <boost/json.hpp>
#include "http_server.h"
#include "model.h"
#include "api.h"
#include <iostream>
#include <filesystem>
#include <variant>
#include <unordered_map>
#include <memory>


namespace request_handler {
namespace net = boost::asio;
namespace sys = boost::system;
namespace beast = boost::beast;
namespace http = beast::http;
namespace fs = std::filesystem;
namespace json = boost::json;


namespace json_responses{

json::array GetRoadsInJSON(const model::Map::Roads& roads);

json::array GetBuildingsInJSON(const model::Map::Buildings& buildings);

json::array GetOfficesInJSON(const model::Map::Offices& offices);

json::object GetPlayersInJSON(const api::Players::PlayerList& players);

std::string MakeMapsList(const model::Game::Maps& maps);

std::string MakeMapDescription(const model::Map* map);

std::string MakeMapNotFound();

std::string MakeErrorCode(std::string_view code, std::string_view message);

} // namespace json_responses

static const std::unordered_map< std::string, std::string> 
FILES_EXTENSIONS {
        {".htm", "text/html"}, {".html", "text/html"}, {".css", "text/css"}, 
        {".txt", "text/plain"}, {".js", "text/javascript"}, {".json", "application/json"}, 
        {".xml", "application/xml"}, {".png", "image/png"}, {".jpg", "image/jpeg"}, 
        {".jpe", "image/jpeg"}, {".jpeg", "image/jpeg"},  {".gif", "image/gif"},
        {".bmp", "image/bmp"}, {".ico", "image/vnd.microsoft.icon"}, {".tiff", "image/tiff"}, 
        {".tif", "image/tiff"},  {".svg", "image/svg+xml"},  {".svgz", "image/svg+xml"},
        {".mp3", "audio/mpeg"}
};

inline std::string ToLower(std::string str){
    std::string result;
    for(char c : str){
        result.push_back(std::tolower(c));
    }

    return result;
}

inline char FromHexToChar(char a, char b){
        a = std::tolower(a);
        b = std::tolower(b);

        if('a' <= a && a <= 'z'){
            a = a - 'a' + 10;
        } else {
            a -= '0';
        }

        if('a' <= b && b <= 'z'){
            b = b - 'a' + 10;
        } else {
            b -= '0';
        }

        return a * 16 + b;
    }

inline std::string DecodeTarget(std::string_view req_target){
    std::string result;
    for(size_t i = 0; i < req_target.size(); ++i){
        if(req_target[i] == '%'){
            if(i + 2 >= req_target.size()){
                return "";
            }
            result.push_back(FromHexToChar(req_target[i + 1], req_target[i + 2]));
            i += 2;
        } else if(req_target[i] == '+'){
            result.push_back(' ');
        } else {
            result.push_back(req_target[i]);
        }
    }  
    return result;
}

inline bool IsMatched(const std::string& str, std::string reg_expression){
    return boost::regex_match(str, boost::regex(reg_expression));
}


using StringResponse = http::response<http::string_body>;
using FileResponse = http::response<http::file_body>;
using VariantResponse = std::variant<StringResponse, FileResponse>;
using Strand = net::strand<net::io_context::executor_type>;

/* 
    Предварительное объявление 
    дружественных классов
    ApiHandler и FileHanlder
    для RequestHandler
*/
class ApiHandler;
class FileHandler;

/* ----------------------------------------------------------- */

class BaseHandler{
protected:
    explicit BaseHandler(){
    }

    StringResponse MakeResponse(http::status status, std::string_view body,
                                        unsigned http_version, size_t content_length, 
                                        std::string content_type){
        StringResponse response(status, http_version);

        response.set(http::field::content_type, content_type);
        response.body() = body;
        response.content_length(content_length);
        return response;
    }

    StringResponse MakeErrorResponse(http::status status, std::string_view code, std::string_view message, unsigned int version){
        using namespace std::literals;

        std::string body = json_responses::MakeErrorCode(code, message);
        return MakeResponse(status, body, version, body.size(), "application/json"s);
    }
};

/* ----------------------------------------------------------- */

class ApiHandler : public BaseHandler{
    friend class RequestHandler;
public:
    template<typename Request>
    StringResponse MakeApiResponse(model::Game& game, Request&& req){
        using namespace std::literals;
        
        std::string target = std::string(req.target());
        if(IsMatched(target, "(/api/v1/maps)"s)){
            return GetMapsListsResponse(game, req.version());
        } else if(IsMatched(target, "(/api/v1/maps/).+"s)) {
            return GetMapDescResponse(game, req);
        } else if(IsMatched(target, "(/api/v1/game/join)"s)){
            auto res = AuthResponse(game, req);
            res.insert("Cache-Control"s, "no-cache"s);
            return res;
        } else if(IsMatched(target, "(/api/v1/game/players)"s)) {
            auto res =  GetPlayersListResponse(req);
            res.insert("Cache-Control"s, "no-cache"s);
            return res;
        }

        return MakeErrorResponse(http::status::bad_request, "badRequest"sv, "Bad request"sv, req.version());
    }
private:
    explicit ApiHandler(Strand api_strand)
        : api_strand_{api_strand}{
    }

    StringResponse GetMapsListsResponse(const model::Game& game, unsigned version){
        using namespace std::literals;

        std::string body = json_responses::MakeMapsList(game.GetMaps());
        return MakeResponse(http::status::ok, body, 
                                        version, body.size(), "application/json"s);
    }

    template<typename Request>
    StringResponse GetMapDescResponse(const model::Game& game, Request&& req){
        using namespace std::literals;

        model::Map::Id id(std::string(req.target().substr(13, req.target().npos)));
        const model::Map* map_ptr = game.FindMap(id);
        if(map_ptr){
            std::string body = json_responses::MakeMapDescription(map_ptr);
            return MakeResponse(http::status::ok, body, 
                                    req.version(), body.size(), "application/json"s);
        }

        std::string body = json_responses::MakeMapNotFound();
        return MakeResponse(http::status::not_found, body,  
                                    req.version(), body.size(), "application/json");
    }

    std::string AuthNewPlayer(const std::string& user_name, const std::string& map_id, model::Game& game, unsigned version){
        using namespace std::literals;
        
        model::GameSession* session = game.SessionIsExists(model::Map::Id(map_id));
        if(session == nullptr){
            session = game.AddSession(model::Map::Id(map_id));
        }
        const model::Dog* dog = session->AddDog(auto_counter_, model::Dog::Name(user_name));
        const api::Player& player = players_.Add(auto_counter_, api::Player::Name(user_name), dog, session);
        ++auto_counter_;

        api::Token token = tokens_.AddPlayer(player);
        
        json::object json_body;
        json_body["authToken"] = *token;
        json_body["playerId"] = player.GetId();

        return json::serialize(json_body);
    }

    template<typename Request>
    StringResponse AuthResponse(model::Game& game, Request&& req){
        using namespace std::literals;
        
        if(req.method_string() == "POST"s){
            if(req.at(http::field::content_type) == "application/json"sv){
                json::object body;
                try{
                    body = json::parse(req.body()).as_object();
                } catch(std::exception& ex){
                    return MakeErrorResponse(http::status::bad_request, 
                        "invalidArgument"sv, "Join game request parse error"sv, req.version());
                }
                
                if(body.count("userName"s) && body.count("mapId")){
                    std::string user_name = std::string(body.at("userName"s).as_string());
                    std::string map_id = std::string(body.at("mapId"s).as_string());

                    if(user_name.empty()){
                        return MakeErrorResponse(http::status::bad_request, 
                            "invalidArgument"sv, "Invalid name"sv, req.version());
                    }

                    if(!game.FindMap(model::Map::Id(map_id))){
                        return MakeErrorResponse(http::status::not_found, 
                            "mapNotFound"sv, "Map not found"sv, req.version());
                    }
                    /* Запрос без ошибок */
                    std::string body = AuthNewPlayer(user_name, map_id, game, req.version());
                    return MakeResponse(http::status::ok, body, req.version(), body.size(), 
                        "application/json"s);
                }
                return MakeErrorResponse(http::status::bad_request, 
                    "invalidArgument"sv, "userName and mapId is expected"sv, req.version());
            }
            return MakeErrorResponse(http::status::bad_request, 
                "invalidArgument"sv, "Content-Type: application/json expected"sv, req.version());
        }
        auto res =  MakeErrorResponse(http::status::method_not_allowed, 
            "invalidMethod"sv, "Only POST method is expected"sv, req.version());
        res.insert("Allow"s, "POST"s);
        return res;
    }

    StringResponse MakePlayerListResponse(unsigned version){
        std::string body = json::serialize(json_responses::GetPlayersInJSON(players_.GetPlayers()));
        return MakeResponse(http::status::ok, body,  
                                    version, body.size(), "application/json");
    }

    template<typename Request>
    StringResponse GetPlayersListResponse(Request&& req){
        using namespace std::literals;

        if(req.method_string() == "GET"s || req.method_string() == "HEAD"s){
            auto it = req.find(http::field::authorization);
            try{
                if(it != req.end()){
                    std::string_view req_token = it->value();
                    api::Token token(std::string(req_token.substr(7, req_token.npos)));
                    if((*token).size() != 32){
                        throw std::logic_error("Incorrect token");
                    }

                    if(tokens_.FindPlayerByToken(token)){
                        return MakePlayerListResponse(req.version());
                    }

                    return MakeErrorResponse(http::status::unauthorized, 
                        "unknownToken"sv, "Player token has not been found"sv, req.version());
                }
            } catch(...){
                return MakeErrorResponse(http::status::unauthorized, 
                    "invalidToken"sv, "Authorization header is missing"sv, req.version());
            }
        }

        auto res =  MakeErrorResponse(http::status::method_not_allowed, 
            "invalidMethod"sv, "Invalid method"sv, req.version());
        res.insert("Allow"s, "GET, HEAD"s);
        return res;
    }

    Strand api_strand_;
    int auto_counter_ = 0;
    api::Players players_;
    api::PlayerTokens tokens_;
};

/* ----------------------------------------------------------- */

class FileHandler : public BaseHandler{
    friend class RequestHandler;
public:
    template <typename Body>
    VariantResponse MakeFileResponse(http::request<Body>&& req){    
        FileResponse response;
        if(req.target() == "/"){
            req.target("/index.html");
        }

        http::file_body::value_type file;
        std::string uncoded_target = DecodeTarget(req.target().substr(1));
        std::string content_type = GetRequiredContentType(req.target());

        fs::path required_path(uncoded_target);
        fs::path summary_path = fs::weakly_canonical(static_path_ / required_path);
        if (sys::error_code ec; file.open(summary_path.string().data(), beast::file_mode::read, ec), ec) {
            std::string empty_body;
            return MakeResponse(http::status::not_found, empty_body,  
                                            req.version(), empty_body.size(), "text/plain");
        }
        
        response.version(req.version());
        response.result(http::status::ok);
        response.insert(http::field::content_type, content_type);
        response.body() = std::move(file);
        // Метод prepare_payload заполняет заголовки Content-Length и Transfer-Encoding
        // в зависимости от свойств тела сообщения
        response.prepare_payload();

        return response;
    }
private:
    explicit FileHandler(fs::path static_path)
        : static_path_(fs::canonical(static_path)){
    }

    std::string GetRequiredContentType(std::string_view req_target){
        auto point = req_target.find_last_of('.');
        std::string extension;
        if(point != req_target.npos){
            extension = ToLower(std::string(req_target.substr(point, req_target.npos)));
        }        
        if(FILES_EXTENSIONS.count(extension)){
            return FILES_EXTENSIONS.at(extension);
        }
        return "application/octet-stream";
    }

    fs::path static_path_;
};

/* ----------------------------------------------------------- */

class RequestHandler : public std::enable_shared_from_this<RequestHandler>{
public:
    explicit RequestHandler(model::Game& game, fs::path static_path, Strand api_strand)
        : game_{game}, file_handler_(static_path), api_handler_{api_strand}{
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template<typename Request, typename Send>
    void operator()(Request&& req, Send&& send) {
        using namespace std::literals;
        // Обработать запрос request и отправить ответ, используя send
    
        /* Api запросы обрабатывает ApiHandler*/
        if(IsMatched(std::string(req.target()), "(/api/).*")){
            auto handle = [self = shared_from_this(), send, req] {
                try {
                    // Этот assert не выстрелит, так как лямбда-функция будет выполняться внутри strand
                    assert(self->api_handler_.api_strand_.running_in_this_thread());
                    return send(self->api_handler_.MakeApiResponse(self->game_, req));
                } catch (...) {
                    send(self->api_handler_.MakeErrorResponse(http::status::bad_request, "badRequest"sv, "Bad request"sv, req.version()));
                }
            };
            return net::dispatch(api_handler_.api_strand_, handle);
        }

        /* Запросы доступа к файлам обрабатывает FileHandler*/
        return std::visit(
                [&send, &req](auto&& result) {
                    send(std::forward<decltype(result)>(result));
                },
                file_handler_.MakeFileResponse(std::forward<decltype(req)>(req)));  
    }

private:

    StringResponse MakeResponse(http::status status, std::string_view body,
                                        unsigned http_version, size_t content_length, 
                                        std::string content_type){
        StringResponse response(status, http_version);

        response.set(http::field::content_type, content_type);
        response.body() = body;
        response.content_length(content_length);
        return response;
    }

    model::Game& game_;
    ApiHandler api_handler_;
    FileHandler file_handler_;
};

}  // namespace request_handler
