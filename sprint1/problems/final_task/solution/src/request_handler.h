#pragma once
#include <boost/regex.hpp>
#include "http_server.h"
#include "model.h"
#include <iostream>

namespace http_handler {
namespace beast = boost::beast;
namespace http = beast::http;

namespace json_responses{

std::string MakeMapsList(const model::Game::Maps& maps);

std::string MakeMapDescription(const model::Map* map);

std::string MakeMapNotFound();

std::string MakeBadRequest();

} // namespace json_responses

class RequestHandler {
public:
    explicit RequestHandler(model::Game& game)
        : game_{game} {
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        // Обработать запрос request и отправить ответ, используя send
        send(HandleRequest(std::forward<decltype(req)>(req)));
    }

private:
    template <typename Body>
    http::response<Body> MakeResponse(http::status status, std::string_view body,
                                        unsigned http_version, size_t content_length, 
                                        std::string content_type){
        http::response<Body> response(status, http_version);

        response.set(http::field::content_type, content_type);
        response.body() = body;
        response.content_length(content_length);
        return response;
    }

    template <typename Body>
    http::response<Body> HandleRequest(http::request<Body>&& req) {
        if(req.method_string() == "GET"){   
            std::string req_target = std::string(req.target());

            if(boost::regex_match(req_target, boost::regex ("(/api/v1/maps)"))){
                std::string body = json_responses::MakeMapsList(game_.GetMaps());
                return MakeResponse<Body>(http::status::ok, body, req.version(), body.size(), std::string("application/json"));
            } else if(boost::regex_match(req_target, boost::regex ("(/api/v1/maps/).+"))) {
                model::Map::Id id(req_target.substr(13, req_target.npos));
                const model::Map* map_ptr = game_.FindMap(id);
                if(map_ptr){
                    std::string body = json_responses::MakeMapDescription(map_ptr);
                    return MakeResponse<Body>(http::status::ok, body, req.version(), body.size(), std::string("application/json"));
                }
                std::string body = json_responses::MakeMapNotFound();
                return MakeResponse<Body>(http::status::not_found, body,  req.version(), body.size(), std::string("application/json"));
            } else if(boost::regex_match(req_target, boost::regex ("(/api/)+"))) {
                std::string body = json_responses::MakeBadRequest();
                return MakeResponse<Body>(http::status::bad_request, body, req.version(), body.size(), std::string("application/json"));
            }
        }
        std::string body = json_responses::MakeBadRequest();
        return MakeResponse<Body>(http::status::bad_request, body, req.version(), body.size(), std::string("application/json"));
    } 

    model::Game& game_;
};

}  // namespace http_handler
