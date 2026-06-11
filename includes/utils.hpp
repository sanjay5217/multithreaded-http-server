#pragma once

#include <iostream>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <cctype>
#include <sstream>

using string_dict = std::unordered_map<std::string, std::string>;
/*
 * Parses the HTTP formatted string and returns all important attributes organized 
 * in a dictionary. 
*/
string_dict extract_string(std::string msg);

