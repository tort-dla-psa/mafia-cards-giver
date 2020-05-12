#pragma once
#include <vector>
#include <string>
#include <sstream>

namespace helpers{
static auto split(const std::string &s){
    std::stringstream ss(s);
    std::vector<std::string> cont;
    std::copy(std::istream_iterator<std::string>(ss),
         std::istream_iterator<std::string>(),
         std::back_inserter(cont));
    return cont;
}
};