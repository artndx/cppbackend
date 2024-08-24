#include "request_handler.h"

namespace request_handler {

namespace detail{

std::string ToLower(std::string str){
    std::string result;
    for(char c : str){
        result.push_back(std::tolower(c));
    }

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
        extension = detail::ToLower(std::string(req_target.substr(point, req_target.npos)));
    }        
    if(detail::FILES_EXTENSIONS.count(extension)){
        return detail::FILES_EXTENSIONS.at(extension);
    }
    return "application/octet-stream";
}

}  // namespace request_handler
