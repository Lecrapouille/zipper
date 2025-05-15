#ifndef GLOB_HPP
#define GLOB_HPP

#include <regex>
#include <string>

std::regex globToRegex(const std::string& glob);

#endif