#pragma once

#include <iostream>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <cctype>
#include <sstream>

/*
 * Parses the HTTP formatted string and returns all important attributes organized 
 * in a dictionary. 
*/
std::unordered_map extract_string(std::string msg);

