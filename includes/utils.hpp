#pragma once

#include <iostream>
#include <string>
#include <functional>
#include <unordered_map>
#include <algorithm>
#include <cctype>
#include <sstream>

using string_dict = std::unordered_map<std::string, std::string>;
// using handle_func = std::function<std::string(const httpRequest&)>;
/*
 * Parses the HTTP formatted string and returns all important attributes organized 
 * in a dictionary. 
*/
string_dict extract_string(std::string msg);

