#pragma once
#include <boost/regex.hpp>
#include "http_server.h"
#include "model.h"
#include <iostream>
#include <filesystem>
#include <variant>
#include <unordered_map>


namespace http_handler {
namespace sys = boost::system;
namespace beast = boost::beast;
namespace http = beast::http;
namespace fs = std::filesystem;

namespace json_responses{

std::string MakeMapsList(const model::Game::Maps& maps);

std::string MakeMapDescription(const model::Map* map);

std::string MakeMapNotFound();

std::string MakeBadRequest();

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
        } else {
            result.push_back(req_target[i]);
        }
    }
    return result;
}


using StringResponse = http::response<http::string_body>;
using FileResponse = http::response<http::file_body>;
using VariantResponse = std::variant<StringResponse, FileResponse>;

class RequestHandler {
public:

    explicit RequestHandler(model::Game& game, fs::path static_path)
        : game_{game}, static_path_(fs::canonical(fs::path(static_path))){
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        // Обработать запрос request и отправить ответ, используя send
        std::visit(
                [&send, &req](auto&& result) {
                    send(std::forward<decltype(result)>(result));
                },
                HandleRequest(std::forward<decltype(req)>(req)));
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

    std::string GetRequiredContentType(std::string_view req_target){
        auto point = req_target.find_last_of('.');
        std::string extension = ToLower(std::string(req_target.substr(point, req_target.npos)));
        if(FILES_EXTENSIONS.count(extension)){
            return FILES_EXTENSIONS.at(extension);
        }
        return "application/octet-stream";
    }

    template <typename Body>
    VariantResponse MakeFileResponse(http::request<Body>&& req){    
        using namespace std::literals;
        FileResponse response;
        if(req.target() == "/"){
            req.target("/index.html");
        }

        http::file_body::value_type file;
        std::string uncoded_target = DecodeTarget(req.target().substr(1));

        fs::path required_path(uncoded_target);
        fs::path summary_path = fs::weakly_canonical(static_path_ / required_path);
        if (sys::error_code ec; file.open(summary_path.string().data(), beast::file_mode::read, ec), ec) {
            std::string empty_body;
            return MakeResponse(http::status::not_found, empty_body,  
                                            req.version(), empty_body.size(), std::string("application/json"));
        }
        
        response.version(req.version());
        response.result(http::status::ok);
        response.insert(http::field::content_type, GetRequiredContentType(req.target()));
        response.body() = std::move(file);
        // Метод prepare_payload заполняет заголовки Content-Length и Transfer-Encoding
        // в зависимости от свойств тела сообщения
        response.prepare_payload();

        return response;
    }

    template <typename Body>
    VariantResponse HandleRequest(http::request<Body>&& req) {
        if(req.method_string() == "GET"){   
            std::string req_target = std::string(req.target());

            if(boost::regex_match(req_target, boost::regex ("(/api/v1/maps)"))){
                std::string body = json_responses::MakeMapsList(game_.GetMaps());
                return MakeResponse(http::status::ok, body, 
                                            req.version(), body.size(), std::string("application/json"));
            } else if(boost::regex_match(req_target, boost::regex ("(/api/v1/maps/).+"))) {
                model::Map::Id id(req_target.substr(13, req_target.npos));
                const model::Map* map_ptr = game_.FindMap(id);
                if(map_ptr){
                    std::string body = json_responses::MakeMapDescription(map_ptr);
                    return MakeResponse(http::status::ok, body, 
                                            req.version(), body.size(), std::string("application/json"));
                }
                std::string body = json_responses::MakeMapNotFound();
                return MakeResponse(http::status::not_found, body,  
                                            req.version(), body.size(), std::string("application/json"));
            } else if(boost::regex_match(req_target, boost::regex ("(/api/)+"))) {
                std::string body = json_responses::MakeBadRequest();
                return MakeResponse(http::status::bad_request, body, 
                                            req.version(), body.size(), std::string("application/json"));
            }

            return MakeFileResponse(std::move(req));
        }
        std::string body = json_responses::MakeBadRequest();
        return MakeResponse(http::status::bad_request, body, 
                                    req.version(), body.size(), std::string("application/json"));
    } 

    model::Game& game_;
    fs::path static_path_;
};

}  // namespace http_handler
