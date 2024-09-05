#include "urldecode.h"

#include <charconv>
#include <stdexcept>


char FromHexToChar(char a, char b){
    a = std::tolower(a);
    b = std::tolower(b);

    if('a' <= a && a <= 'f'){
        a = a - 'a' + 10;
    } else if('2' <= a && a <= '9'){
        a -= '0';
    } else {
        throw std::invalid_argument("Incorrect URL");
    }

    if('a' <= b && b <= 'f'){
        b = b - 'a' + 10;
    } else if('0' <= b && b <= '9'){
        b -= '0';
    } else {
        throw std::invalid_argument("Incorrect URL");
    }

    return a * 16 + b;
}

std::string UrlDecode(std::string_view str) {
    std::string result;
    for(size_t i = 0; i < str.size(); ++i){
        if(str[i] == '%'){
            if(i + 2 >= str.size()){
                throw std::invalid_argument("Incorrect URL");
            }
            char encode_char = FromHexToChar(str[i + 1], str[i + 2]);
            result.push_back(encode_char);
            i += 2;
        } else if(str[i] == '+'){
            result.push_back(' ');
        } else {
            result.push_back(str[i]);
        }
    }  
    return result;
}
