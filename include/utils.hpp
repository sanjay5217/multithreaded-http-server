#pragma once

#include <iostream>
#include <string>
#include <functional>
#include <unordered_map>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <random> 
#include <mutex>

using string_dict = std::unordered_map<std::string, std::string>;

/**
 * @brief Handles the client message are responds back appropriately
 *
 * @param client_fd Client socket's fd
 * @param router Router object reference
 */
//void handle_client(int client_fd, Router& router);

/**
 * @brief Parses an HTTP message and extracts its attributes.
 *
 * @param msg Raw HTTP message as a string.
 * @return A string_dict containing parsed key-value attributes 
 *          (e.g., headers, method, path).
 */
string_dict extract_string(std::string msg);

/**
 * @brief Computes the n-th Fibonacci number using an inefficient approach.
 *
 * @param n Index in the Fibonacci sequence.
 * @return The n-th Fibonacci number.
 */
int fibonacci(int n);

/**
 * @brief Generates a random integer in the range [a, b).
 *
 * @param a Lower bound, inclusive.
 * @param b Upper bound, exclusive.
 * @return A random integer in the range [a, b).
 */
int random_int(int a, int b);

/**
 * @brief Looks up a header by name, case-insensitively, without altering
 *        how headers are stored
 *
 * @param headers The header map to search.
 * @param key The header name to look up.
 * @return The matching value, or "" if no header matches.
 */
std::string get_header_ci(const string_dict& headers, const std::string& key);
