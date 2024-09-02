#include "request_handler.h"
#include <algorithm>

namespace request_handler {

namespace detail{

std::string ToLower(std::string_view str){
    std::string result;
    result.resize(str.size());
    std::transform(str.begin(), str.end(), result.begin(), [](char c){
        return std::tolower(c);
    });

    return result;
}

char FromHexToChar(char a, char b){
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

std::string DecodeTarget(std::string_view req_target){
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

std::string MakeErrorCode(std::string_view code, std::string_view message){
    json::object body;
    body["code"] = std::string(code);
    body["message"] = std::string(message);
    return json::serialize(body);
}

bool IsMatched(const std::string& str, std::string reg_expression){
    return boost::regex_match(str, boost::regex(reg_expression));
}

Milliseconds FromDouble(double delta){
    return std::chrono::duration_cast<Milliseconds>(std::chrono::duration<double>(delta/1000));
}

/* ------------------------ Ticker ----------------------------------- */

void Ticker::Start() {
    net::dispatch(strand_, [self = shared_from_this()] {
        self->last_tick_ = Clock::now();
        self->ScheduleTick();
    });
}

void Ticker::ScheduleTick() {
    assert(strand_.running_in_this_thread());
    timer_.expires_after(period_);
    timer_.async_wait([self = shared_from_this()](sys::error_code ec) {
        self->OnTick(ec);
    });
}

void Ticker::OnTick(sys::error_code ec) {
    using namespace std::chrono;
    assert(strand_.running_in_this_thread());

    if (!ec) {
        auto this_tick = Clock::now();
        auto delta = duration_cast<milliseconds>(this_tick - last_tick_);
        last_tick_ = this_tick;
        handler_(delta);
        ScheduleTick();
    }
}

} // namespace detail

/* ------------------------ BaseHandler ----------------------------------- */

StringResponse BaseHandler::MakeResponse(http::status status, std::string_view body,
                                    unsigned http_version, size_t content_length, 
                                    std::string content_type){
    StringResponse response(status, http_version);

    response.set(http::field::content_type, content_type);
    response.set(http::field::cache_control, "no-cache"s);
    response.body() = body;
    response.content_length(content_length);
    return response;
}

StringResponse BaseHandler::MakeErrorResponse(http::status status, std::string_view code, 
                                        std::string_view message, unsigned int version){
    using namespace std::literals;

    std::string body = detail::MakeErrorCode(code, message);
    return MakeResponse(status, body, version, body.size(), "application/json"s);
}

/* -------------------------- ApiHandler --------------------------------- */

StringResponse ApiHandler::MakeMapsListsResponse(unsigned version){
    using namespace std::literals;

    std::string body = app_.GetMapsList();
    return MakeResponse(http::status::ok, body, 
                                    version, body.size(), "application/json"s);
}

StringResponse ApiHandler::MakeMapDescResponse(const std::string& req_target, unsigned req_version){
    using namespace std::literals;

    model::Map::Id id(std::string(req_target.substr(13, req_target.npos)));
    if(auto map = app_.FindMap(id); map){
        std::string body = app_.GetMapDescription(map);
        return MakeResponse(http::status::ok, body, 
                                req_version, body.size(), "application/json"s);
    }

    return MakeErrorResponse(http::status::not_found, 
        "mapNotFound"sv, "Map not found"sv, req_version);
}

/* -------------------------- FileHandler --------------------------------- */

std::string FileHandler::GetRequiredContentType(std::string_view req_target){
    auto point = req_target.find_last_of('.');
    std::string extension;
    if(point != req_target.npos){
        extension = detail::ToLower(req_target.substr(point, req_target.npos));
    }        
    if(detail::FILES_EXTENSIONS.count(extension)){
        return detail::FILES_EXTENSIONS.at(extension);
    }
    return "application/octet-stream";
}

}  // namespace request_handler