#pragma once

#include <memory>
#include <string>
#include <unordered_map>

std::unique_ptr<std::unordered_map<std::string, std::string>> readJar(const char* filename);
